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

#ifndef CUDA_AUTOCOMPAT_SEARCH_SEARCH_H
#define CUDA_AUTOCOMPAT_SEARCH_SEARCH_H

#include <sys/types.h>

#include <filesystem>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace autocompat {

struct SearchResult {
    int version;
    std::filesystem::path driver_dir;
};

struct SearchState {
    std::optional<SearchResult> found;
    std::unordered_set<std::filesystem::path> dir_path_cache;
    std::unordered_set<ino_t> dir_inode_cache;
    std::unordered_map<ino_t, int> ver_cache;
};

void search_libraries_libcuda(const std::vector<std::filesystem::path> &libs,
                              SearchState &state);

void search_libraries_libcudart(const std::vector<std::filesystem::path> &libs,
                                SearchState &state);

void search_cuda_home(SearchState &state);

void search_paths_libcudart(const std::vector<std::filesystem::path> &paths,
                            SearchState &state);

void search_paths_libcuda(const std::vector<std::filesystem::path> &paths,
                          SearchState &state);

} // namespace autocompat

#endif // CUDA_AUTOCOMPAT_SEARCH_SEARCH_H
