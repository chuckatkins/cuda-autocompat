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

#include <cstdlib>

#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "logging.h"
#include "version.h"

#include "search.h"

namespace autocompat {

void init_logging(void);

bool parse_args(std::span<char *> argv,
                std::vector<std::filesystem::path> &search_paths,
                std::vector<std::filesystem::path> &search_libs);

} // namespace autocompat

inline auto parse_libcuda_version(int ver) {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    return std::to_array({ver / 1000, (ver % 100) / 10, ver % 10});
}

int main(int argc, char *argv[]) {
    using namespace autocompat;

    init_logging();

    log_info("CUDA AutoCompat v{}", CUDA_AUTOCOMPAT_VERSION_STRING);

    std::vector<std::filesystem::path> search_paths;
    std::vector<std::filesystem::path> search_libs;
    if (!parse_args({argv, static_cast<size_t>(argc)}, search_paths,
                    search_libs)) {
        return EXIT_FAILURE;
    }

    SearchState state;

    log_info("Searching for best available libcuda.so.1");

    search_libraries_libcuda(search_libs, state);
    if (!state.found) {
        search_libraries_libcudart(search_libs, state);
        search_cuda_home(state);
        search_paths_libcudart(search_paths, state);
        search_paths_libcuda(search_paths, state);
    }

    log_info("Search complete");

    if (state.found) {
        const auto found_ver = parse_libcuda_version(state.found->version);
        log_info("Found library: {}/libcuda.so.1", state.found->driver_dir);
        log_info("Found version: {}.{}.{}", found_ver[0], found_ver[1], found_ver[2]);
        std::cout << state.found->driver_dir.native() << std::flush;
        return EXIT_SUCCESS;
    }
    log_info("No usable library found");

    return EXIT_FAILURE;
}
