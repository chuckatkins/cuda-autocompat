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

#ifndef CUDA_AUTOCOMPAT_TESTS_STUBS_CUDA_ERROR_H_
#define CUDA_AUTOCOMPAT_TESTS_STUBS_CUDA_ERROR_H_

// This is part of a stub implementation of the CUDA driver library for testing.

typedef enum {
    CUDA_SUCCESS = 0,
    CUDA_ERROR_INVALID_VALUE = 1,
    CUDA_ERROR_UNKNOWN = 999
} CUresult;

#endif // CUDA_AUTOCOMPAT_TESTS_STUBS_CUDA_ERROR_H_
