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

// This is part of a stub implementation of the CUDA driver library for testing.

#include "cuda_error.h"

#include <stddef.h>

#include "visibility.h"

static const char name_success[] = "CUDA_SUCCESS";
static const char string_success[] = "no error";

static const char name_invalid[] = "CUDA_ERROR_INVALID_VALUE";
static const char string_invalid[] = "invalid argument";

static const char name_unknown[] = "CUDA_ERROR_UNKNOWN";
static const char string_unknown[] = "unknown error";

#define get_error_impl(error, pStr, msg_type) \
    if ((pStr) == NULL) {                     \
        return CUDA_ERROR_INVALID_VALUE;      \
    }                                         \
    switch (error) {                          \
    case CUDA_SUCCESS:                        \
        *(pStr) = msg_type##_success;         \
        return CUDA_SUCCESS;                  \
    case CUDA_ERROR_INVALID_VALUE:            \
        *(pStr) = msg_type##_invalid;         \
        return CUDA_SUCCESS;                  \
    case CUDA_ERROR_UNKNOWN:                  \
        *(pStr) = msg_type##_unknown;         \
        return CUDA_SUCCESS;                  \
    default:                                  \
        *(pStr) = NULL;                       \
        return CUDA_ERROR_INVALID_VALUE;      \
    }

DLL_PUBLIC
CUresult cuGetErrorName(CUresult error, const char **pStr) {
    get_error_impl(error, pStr, name);
}

DLL_PUBLIC
CUresult cuGetErrorString(CUresult error, const char **pStr) {
    get_error_impl(error, pStr, string);
}
