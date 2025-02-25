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
#ifndef CUDA_AUTOCOMPAT_SEARCH_LOGGING_H
#define CUDA_AUTOCOMPAT_SEARCH_LOGGING_H

#include <cstddef>

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace autocompat {

enum class log_level : int {
    off = 0,
    error,
    warn,
    info,
    verbose,
    debug,
    trace,
};

inline log_level LOGGING_MAX_LEVEL = log_level::warn;
inline bool LOGGING_LONG_LEVEL_NAME = false;
inline std::string LOGGING_LOG_NAME = "main";
inline bool LOGGING_USE_TIMESTAMP = true;
inline bool LOGGING_USE_LOG_NAME = true;
inline bool LOGGING_USE_LEVEL_NAME = true;

namespace logging_details {
using namespace std::literals::string_view_literals;
constexpr auto log_level_names = std::to_array({
    "OFF"sv,
    "ERROR"sv,
    "WARN"sv,
    "INFO"sv,
    "VERBOSE"sv,
    "DEBUG"sv,
    "TRACE"sv,
});

constexpr auto log_level_name_max_len =
    std::ranges::max(log_level_names, {}, &std::string_view::size).size();

constexpr auto log_level_max_indent = [] {
    std::array<char, log_level_names.size() * 2> arr{};
    for (auto &element : arr) {
        element = ' ';
    }
    return arr;
}();

consteval const std::string_view &to_string(log_level level) {
    return log_level_names[static_cast<size_t>(level)];
}

static constexpr std::string empty_string;

inline auto gen_log_header_timestamp(void) {
    if (LOGGING_USE_TIMESTAMP) {
        return std::format(
            "{:%FT%T} ",
            std::chrono::floor<std::chrono::system_clock::duration>(
                std::chrono::system_clock::now()));
    }

    return empty_string;
}

inline auto gen_log_header_log_name(void) {
    if (LOGGING_USE_LOG_NAME) {
        return std::format("{} ", LOGGING_LOG_NAME);
    }
    return empty_string;
}

template <log_level level> inline auto get_log_header_level_name(void) {
    if (LOGGING_USE_LEVEL_NAME) {
        if (LOGGING_LONG_LEVEL_NAME) {
            return std::format("{:<{}} ", to_string(level),
                               log_level_name_max_len);
        } else {
            return std::format("{} ", to_string(level)[0]);
        }
    }
    return empty_string;
}

template <log_level level, typename... Args>
void log_write(std::format_string<Args...> fmt, Args &&...args) {
    if (level <= LOGGING_MAX_LEVEL) {
        const auto msg_timestamp = gen_log_header_timestamp();
        const auto msg_log_name = gen_log_header_log_name();
        const auto msg_level_name = get_log_header_level_name<level>();
        static constexpr std::string_view indent{
            log_level_max_indent.data(),
            (static_cast<size_t>(std::max(log_level::info, level)) -
             static_cast<size_t>(log_level::info)) *
                2};

        const auto msg = std::format(fmt, std::forward<Args>(args)...);
        std::cerr << std::format("{}{}{}{}{}\n", msg_timestamp, msg_log_name,
                                 msg_level_name, indent, msg);
    }
}
} // namespace logging_details

template <typename... Args>
inline void log_error(std::format_string<Args...> fmt, Args &&...args) {
    logging_details::log_write<log_level::error>(fmt,
                                                 std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_warn(std::format_string<Args...> fmt, Args &&...args) {
    logging_details::log_write<log_level::warn>(fmt,
                                                std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_info(std::format_string<Args...> fmt, Args &&...args) {
    logging_details::log_write<log_level::info>(fmt,
                                                std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_verbose(std::format_string<Args...> fmt, Args &&...args) {
    logging_details::log_write<log_level::verbose>(fmt,
                                                   std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_debug(std::format_string<Args...> fmt, Args &&...args) {
    logging_details::log_write<log_level::debug>(fmt,
                                                 std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_trace(std::format_string<Args...> fmt, Args &&...args) {
#ifndef NDEBUG
    logging_details::log_write<log_level::trace>(fmt,
                                                 std::forward<Args>(args)...);
#endif
}

} // end namespace autocompat

// Note: This is a basic use case essentially just using the underlying string
// formatter but it's good enough for our use case.  Both fmtlib and std
// library implementations after C++26 will be more robust. The NOLINT
// suppression is to ignore warnings about modifying the std namespace
#ifndef __cpp_lib_format_path
    #include <filesystem>
template <class CharT>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::formatter<std::filesystem::path, CharT>
    : public std::formatter<std::filesystem::path::string_type, CharT> {
    template <class FormatContext>
    auto format(const std::filesystem::path &arg, FormatContext &ctx) const {
        return std::formatter<std::filesystem::path::string_type,
                              CharT>::format(arg.native(), ctx);
    }
};
#endif // __cpp_lib_format_path

#endif // CUDA_AUTOCOMPAT_SEARCH_LOGGING_H
