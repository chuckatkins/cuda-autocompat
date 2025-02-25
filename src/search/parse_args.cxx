/* Copyright 2025 Chuck Atkins and CUDA Auto-Compat contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#include <dlfcn.h>
#include <getopt.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <iostream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_set>

#include "logging.h"

namespace autocompat {

namespace {

struct CmdFlag {
    char opt_short = '\0';
    std::string_view opt_long;
    std::string_view opt_value;
    std::string_view opt_help;
};

constexpr std::array CMD_FLAGS = {
    CmdFlag{'p', "search-path", "PATH", "Colon-separated library search path."},
    CmdFlag{'l', "libs", "LIBRARIES",
            "Colon-separated library list to search."},
    CmdFlag{'h', "help", "", "Display this help and exit."}};

// Helper function for usage; determine the maximum formatted length for long
// options
consteval size_t get_long_opt_max_len(void) {
    size_t max_len = 0;
    for (const auto &flag : CMD_FLAGS) {
        if (!flag.opt_long.empty()) {
            size_t len = 2 + flag.opt_long.size(); // "--long-opt"
            if (!flag.opt_value.empty()) {
                len += 1 + flag.opt_value.size(); // "--long-opt=value"
            }
            max_len = std::max(max_len, len);
        }
    }
    return max_len;
}

void usage(const std::string_view exe) {
    constexpr size_t long_opt_max_len = get_long_opt_max_len();

    log_error("Usage: {} [OPTIONS]", exe);

    for (const auto &flag : CMD_FLAGS) {
        std::string short_opt =
            flag.opt_short != '\0' ? std::format("-{}", flag.opt_short) : "";

        std::string long_opt =
            flag.opt_long.empty() ? ""
            : flag.opt_value.empty()
                ? std::format("--{}", flag.opt_long)
                : std::format("--{}={}", flag.opt_long, flag.opt_value);

        log_error("  {:2}{} {:<{}} {}", short_opt,
                  (short_opt.empty() || long_opt.empty()) ? ' ' : ',', long_opt,
                  long_opt_max_len, flag.opt_help);
    }
}

// Helper function for parse_args; generate the buffer used for the getopt_long
// shortopts string at compile time
consteval auto generate_shortopts() {
    constexpr size_t max_len = 1 + (CMD_FLAGS.size() * 2) + 1;

    std::array<char, max_len> buf{};
    char *out = buf.data();

    *out++ = ':';
    for (const auto &flag : CMD_FLAGS) {
        if (flag.opt_short != '\0') {
            *out++ = flag.opt_short;
            if (!flag.opt_value.empty()) {
                *out++ = ':'; // Indicates required argument
            }
        }
    }

    // buf is value initialized to '\0' so no need for null explicit
    // termination

    return buf;
}

// Helper function for parse_args; generate the getopt_long longopts array at
// compile time
consteval std::array<struct option, CMD_FLAGS.size() + 1>
generate_longopts(void) {
    // +1 for the terminating entry
    std::array<option, CMD_FLAGS.size() + 1> longopts{};

    std::ranges::transform(CMD_FLAGS, longopts.begin(), [](const auto &flag) {
        // flag.opt_long is constructed using string literals and thus is
        // guaranteed to be a null-terminated string
        // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
        return option{flag.opt_long.data(),
                      flag.opt_value.empty() ? no_argument : required_argument,
                      nullptr, flag.opt_short};
    });

    // Null terminator
    longopts[CMD_FLAGS.size()] = {nullptr, 0, nullptr, 0};

    return longopts;
}

void add_path(const std::string_view src,
              std::vector<std::filesystem::path> &out,
              std::unordered_set<std::filesystem::path> &cache, bool dir_mode) {
    std::filesystem::path src_path;

    if (src.empty()) {
        if (!dir_mode) {
            log_debug("skip empty");
            return;
        }
        src_path = ".";
    } else {
        src_path = src;
    }

    if (!(cache.insert(src_path).second)) {
        log_debug("skip {} (already processed)", src_path);
        return;
    }

    if (!std::filesystem::exists(src_path)) {
        log_debug("skip {} (does not exist)", src_path);
        return;
    }
    if (dir_mode && !std::filesystem::is_directory(src_path)) {
        log_debug("skip {} (not a directory)", src_path);
        return;
    }
    if (!dir_mode && !std::filesystem::is_regular_file(src_path)) {
        log_debug("skip {} (not a regular file)", src_path);
        return;
    }

    log_verbose("{}", src_path);
    out.push_back(src_path);
}

void parse_paths(const std::string_view src,
                 std::vector<std::filesystem::path> &dst,
                 std::unordered_set<std::filesystem::path> &cache,
                 bool dir_mode) {
    if (src.empty()) {
        return;
    }
    dst.reserve(dst.size() + std::ranges::count(src, ':') + 1);

    size_t cur = 0;
    size_t next = std::string_view::npos;
    while ((next = src.find(':', cur)) != std::string_view::npos) {
        add_path(src.substr(cur, next - cur), dst, cache, dir_mode);
        cur = next + 1;
    }
    add_path(cur == 0 ? src : src.substr(cur), dst, cache, dir_mode);
}

// Get the default search path from the dynamic linker for when the arguments
// don't specify one.
bool get_default_search_path(std::vector<std::filesystem::path> &out,
                             std::unordered_set<std::filesystem::path> &cache) {
    void *handle = dlopen(nullptr, RTLD_LAZY | RTLD_LOCAL);
    if (handle == nullptr) {
        log_trace("{}", dlerror());
        return false;
    }

    Dl_serinfo serinfo_size;
    if (dlinfo(handle, RTLD_DI_SERINFOSIZE, &serinfo_size) != 0) {
        log_trace("{}", dlerror());
        dlclose(handle);
        return false;
    }
    if (serinfo_size.dls_cnt == 0) {
        dlclose(handle);
        return false;
    }

    std::vector<char> serinfo_buf(serinfo_size.dls_size, '\0');
    // safe system-level struct aliasing
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *serinfo = reinterpret_cast<Dl_serinfo *>(serinfo_buf.data());

    serinfo->dls_size = serinfo_size.dls_size;
    serinfo->dls_cnt = serinfo_size.dls_cnt;

    if (dlinfo(handle, RTLD_DI_SERINFO, serinfo) != 0) {
        log_trace("{}", dlerror());
        dlclose(handle);
        return false;
    }
    dlclose(handle);

    out.reserve(out.size() + serinfo->dls_cnt);
    const std::span<Dl_serpath> dls_serpath{
        static_cast<Dl_serpath *>(serinfo->dls_serpath), serinfo->dls_cnt};
    for (const auto &serpath : dls_serpath) {
        add_path(serpath.dls_name, out, cache, true);
    }

    return true;
}

std::vector<std::string> parse_argv_from_stdin(void) {
    std::string line;
    if (!std::getline(std::cin, line)) {
        log_error("Failed to read arguments from stdin");
        return {};
    }
    log_debug("{}", line);

    std::vector<std::string> args;
    std::istringstream iss(line);
    for (std::string arg; iss >> arg;) {
        log_trace("{}", arg);
        args.push_back(arg);
    }

    return args;
}

bool parse_args_helper(std::span<char *> argv,
                       std::vector<std::filesystem::path> &paths,
                       std::unordered_set<std::filesystem::path> &path_cache,
                       std::vector<std::filesystem::path> &libs,
                       std::unordered_set<std::filesystem::path> &lib_cache,
                       bool &arg_search_path_seen) {

    static constexpr auto optstring = generate_shortopts();
    static constexpr auto longopts = generate_longopts();

    // Ensure getopt state is explicitly initialized / reset so
    // parse_args_helper can be called recursively
    optind = 1;
    optarg = nullptr;

    int opt = -1;
    while ((opt = getopt_long(static_cast<int>(argv.size()), argv.data(),
                              optstring.data(), longopts.data(), nullptr)) !=
           -1) {
        switch (opt) {
        case 'p':
            arg_search_path_seen = true;
            log_info("Adding search paths");
            parse_paths(optarg, paths, path_cache, true);
            break;
        case 'l':
            log_info("Adding search libs");
            parse_paths(optarg, libs, lib_cache, false);
            break;
        case 'h':
            usage(argv[0]);
            return false;
        case '?':
            log_error("{}: unrecognized option '{}'", argv[0],
                      argv[optind - 1]);
            return false;
        case ':':
            log_error("{}: option '{}' requires an argument", argv[0],
                      argv[optind - 1]);
            return false;
        default:
            log_error("{}: option '{}' error", argv[0], argv[optind - 1]);
            return false;
        }
    }

    if (optind < static_cast<int>(argv.size())) {
        for (auto *const arg : argv.subspan(optind)) {
            if (std::string_view(arg) != "-") {
                log_error("{}: unrecognized argument '{}'", argv[0], arg);
                return false;
            }
            log_info("Reading additional arguments from stdin");
            auto new_args = parse_argv_from_stdin();
            if (new_args.empty()) {
                return false;
            }

            std::vector<char *> new_argv;
            new_argv.reserve(new_args.size() + 1);
            new_argv.push_back(argv[0]);
            std::ranges::transform(new_args, std::back_inserter(new_argv),
                                   [](std::string &str) { return str.data(); });

            if (!parse_args_helper(new_argv, paths, path_cache, libs, lib_cache,
                                   arg_search_path_seen)) {
                return false;
            }
        }
    }

    return true;
}

} // end anonymous namespace

bool parse_args(std::span<char *> argv,
                std::vector<std::filesystem::path> &paths,
                std::vector<std::filesystem::path> &libs) {
    bool arg_search_path_seen = false;
    std::unordered_set<std::filesystem::path> path_cache;
    std::unordered_set<std::filesystem::path> lib_cache;

    if (!parse_args_helper(argv, paths, path_cache, libs, lib_cache,
                           arg_search_path_seen)) {
        return false;
    }

    if (!arg_search_path_seen) {
        log_info("Adding default search paths");
        if (!get_default_search_path(paths, path_cache)) {
            log_error("failed to get default search path.");
            return false;
        }
    }

    return true;
}

} // namespace autocompat
