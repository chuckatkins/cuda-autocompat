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

#include "search.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include <array>
#include <functional>
#include <string>
#include <utility>

#include "dl_library.h"
#include "logging.h"

namespace autocompat {

namespace {

int get_libcuda_api_ver(const std::filesystem::path &libcuda_path,
                        SearchState &state) {
    struct stat libcuda_stat{};

    log_trace("stat({})", libcuda_path);
    if (::stat(libcuda_path.c_str(), &libcuda_stat) != 0) {
        log_trace("{}", std::strerror(errno));
        return -3;
    }
    if (S_ISDIR(libcuda_stat.st_mode)) {
        return -4;
    }

    auto cache_entry = state.ver_cache.emplace(libcuda_stat.st_ino, -1);
    if (!cache_entry.second) {
        log_debug("cached (inode = {})", libcuda_stat.st_ino);
        return cache_entry.first->second;
    }

    const DlLibrary libcuda{libcuda_path};
    if (!libcuda) {
        return -1;
    }

    if (libcuda.get_data_symbol<int>("cuda_autocompat_version") != nullptr) {
        return -2;
    }

    auto cuGetErrorName =
        libcuda.get_function_symbol<int, int, const char *&>("cuGetErrorName");
    if (!cuGetErrorName) {
        return -1;
    }

    auto cuGetErrorString =
        libcuda.get_function_symbol<int, int, const char *&>(
            "cuGetErrorString");
    if (!cuGetErrorString) {
        return -1;
    }

    auto cuDriverGetVersion =
        libcuda.get_function_symbol<int, int &>("cuDriverGetVersion");
    if (!cuDriverGetVersion) {
        return -1;
    }

    int &ver = cache_entry.first->second;
    int ret = cuDriverGetVersion(ver);
    if (ret != 0) {
        const char *err_name = nullptr;
        const char *err_string = nullptr;
        if (cuGetErrorName(ret, err_name) == 0 &&
            cuGetErrorString(ret, err_string) == 0) {
            log_trace("cuDriverGetVersion: {} ({})", err_name, err_string);
        } else {
            log_trace("cuDriverGetVersion: {}", ret);
        }
    }

    return ver;
}

inline bool check_file_exists(const std::filesystem::path &file_path) {
    auto file_stat = std::filesystem::status(file_path);
    return std::filesystem::exists(file_stat) &&
           std::filesystem::is_regular_file(file_stat);
}

int update_libcuda(const std::filesystem::path &libcuda_path,
                   SearchState &state) {
    log_info("libcuda: {}", libcuda_path);

    auto libcuda_dir = libcuda_path.parent_path();
    if (!state.dir_path_cache.insert(libcuda_dir).second) {
        log_info("libcuda: Skipping (directory already checked)");
        return -1;
    }

    struct stat libcuda_dir_stat{};
    log_trace("stat({})", libcuda_dir);
    if (::stat(libcuda_dir.c_str(), &libcuda_dir_stat) != 0) {
        log_trace("{}", std::strerror(errno));
        log_info("libcuda: Skipping (directory stat error)");
        return -1;
    }
    if (!S_ISDIR(libcuda_dir_stat.st_mode)) {
        log_info("libcuda: Skipping (directory error)");
        return -1;
    }
    if (!state.dir_inode_cache.insert(libcuda_dir_stat.st_ino).second) {
        log_debug("cached (inode = {})", libcuda_dir_stat.st_ino);
        log_info("libcuda: Skipping (directory inode already checked)");
        return -1;
    }
    
    int ver = get_libcuda_api_ver(libcuda_path, state);
    switch (ver) {
    case -4:
        log_info("libcuda: Skipping (directory)");
        return -1;
    case -3:
        log_info("libcuda: Skipping (stat error)");
        return -1;
    case -2:
        log_info("libcuda: Skipping (autocompat detected)");
        return -1;
    case -1:
        log_info("libcuda: Skipping (library error)");
        return -1;
    default:
        break;
    }

    if (!check_file_exists(libcuda_dir / "libnvidia-nvvm.so.4")) {
        log_info("libcuda: Skipping (libnvidia-nvvm.so.4 not found)");
        return -1;
    }
    if (!check_file_exists(libcuda_dir / "libnvidia-ptxjitcompiler.so.1")) {
        log_info("libcuda: Skipping (libnvidia-nvvm.so.4 not found)");
        return -1;
    }
    if (!check_file_exists(libcuda_dir / "libcudadebugger.so.1")) {
        log_info("libcuda: Skipping (libcudadebugger.so.1 not found)");
        return -1;
    }

    log_info("libcuda: cuDriverGetVersion = {}", ver);

    if (!state.found) {
        log_info("libcuda: Updating (first found)");
        state.found = {ver, libcuda_dir};
        return 0;
    }

    if (ver > state.found->version) {
        log_info("libcuda: Updating ({} > {})", ver, state.found->version);
        state.found = {ver, libcuda_dir};
        return 0;
    }

    log_info("libcuda: Skipping ({} <= {})", ver, state.found->version);

    return 1;
}

inline std::optional<std::filesystem::path>
check_path_ends_with(std::filesystem::path full, std::filesystem::path suffix) {
    while (!suffix.empty()) {
        if (full.empty()) {
            return std::nullopt;
        }
        if (full.filename() != suffix.filename()) {
            return std::nullopt;
        }
        full = full.parent_path();
        suffix = suffix.parent_path();
    }
    return full;
}

inline std::optional<std::filesystem::path>
get_toolkit_from_libcudart(const std::filesystem::path &lib_path) {
    const auto &reallib_dir =
        std::filesystem::weakly_canonical(lib_path).parent_path();
    log_debug("-> {}", reallib_dir);

    constexpr auto toolkit_subdir = "targets/x86_64-linux/lib";
    return check_path_ends_with(reallib_dir, toolkit_subdir);
}

} // end anonymous namespace

void search_libraries_libcuda(const std::vector<std::filesystem::path> &libs,
                           SearchState &state) {
    log_info("Searching for driver in libraries");
    for (const auto &lib_path : libs) {
        log_verbose("{}", lib_path);
        if (lib_path.filename() == "libcuda.so.1" &&
            update_libcuda(lib_path, state) >= 0) {
            break;
        }
    }
}

void search_libraries_libcudart(const std::vector<std::filesystem::path> &libs,
                             SearchState &state) {
    log_info("Searching for toolkits in libraries");
    for (const auto &libcudart_path : libs) {
        log_verbose("{}", libcudart_path);
        const auto libcudart_fname = libcudart_path.filename();
        if (libcudart_fname == "libcudart.so.11" ||
            libcudart_fname == "libcudart.so.12" ||
            libcudart_fname == "libcudart.so.13") {
            const auto toolkit_dir = get_toolkit_from_libcudart(libcudart_path);
            if (!toolkit_dir) {
                continue;
            }
            log_debug("-> {}", *toolkit_dir);
            const auto libcuda_path = *toolkit_dir / "compat" / "libcuda.so.1";
            (void)update_libcuda(libcuda_path, state);
        }
    }
}

void search_cuda_home(SearchState &state) {
    log_info("Searching for toolkit in CUDA_HOME");
    const char *env_value = secure_getenv("CUDA_HOME");
    if (env_value == nullptr) {
        return;
    }

    const std::filesystem::path toolkit_dir(env_value);
    log_verbose("CUDA_HOME={}", toolkit_dir);

    const auto libcuda_path = toolkit_dir / "compat" / "libcuda.so.1";
    if (!check_file_exists(libcuda_path)) {
        return;
    }
    (void)update_libcuda(libcuda_path, state);
}

void search_paths_libcudart(const std::vector<std::filesystem::path> &paths,
                              SearchState &state) {
    log_info("Searching for toolkits in library search path");
    constexpr auto libcudart_soname = std::to_array(
        {"libcudart.so.11", "libcudart.so.12", "libcudart.so.13"});
    for (auto const &libcudart_dir : paths) {
        log_verbose("{}", libcudart_dir);
        for (auto const &libcudart_fname : libcudart_soname) {
            const auto libcudart_path = libcudart_dir / libcudart_fname;
            log_debug("{}", libcudart_path);
            if (!check_file_exists(libcudart_path)) {
                continue;
            }
            const auto toolkit_dir = get_toolkit_from_libcudart(libcudart_path);
            if (!toolkit_dir) {
                continue;
            }
            log_debug("-> {}", *toolkit_dir);
            const auto libcuda_path = *toolkit_dir / "compat" / "libcuda.so.1";
            if (check_file_exists(libcuda_path)) {
                (void)update_libcuda(libcuda_path, state);
            }
            break;
        }
    }
}

void search_paths_libcuda(const std::vector<std::filesystem::path> &paths,
                            SearchState &state) {
    log_info("Searching for driver in library search path");
    for (auto const &lib_dir : paths) {
        log_verbose("{}", lib_dir);
        const auto lib_path = lib_dir / "libcuda.so.1";
        log_debug("{}", lib_path);
        if (!check_file_exists(lib_path)) {
            continue;
        }
        (void)update_libcuda(lib_path, state);
    }
}

} // namespace autocompat
