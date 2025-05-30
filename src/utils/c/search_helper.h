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
#ifndef CUDA_AUTOCOMPAT_UTILS_C_SEARCH_HELPER_H
#define CUDA_AUTOCOMPAT_UTILS_C_SEARCH_HELPER_H

#include <limits.h>
#include <stdbool.h>

bool find_search_helper(char out_path[PATH_MAX]);
size_t find_libcuda(char out_path[PATH_MAX]);

#endif // CUDA_AUTOCOMPAT_UTILS_C_SEARCH_HELPER_H
