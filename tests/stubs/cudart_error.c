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

// This is part of a stub implementation of the CUDA runtime library for
// testing.

#include "cudart_error.h"

#include "visibility.h"

static const char name_success[] = "cudaSuccess";
static const char string_success[] = "no error";

static const char name_insufficient_driver[] = "cudaErrorInsufficientDriver";
static const char string_insufficient_driver[] = "CUDA driver version is insufficient for CUDA runtime version";

static const char name_invalid[] = "cudaErrorInvalidValue";
static const char string_invalid[] = "invalid argument";

static const char name_driver_mismatch[] = "cudaErrorSystemDriverMismatch";
static const char string_driver_mismatch[] = "system has unsupported display driver / cuda driver combination";

static const char name_unknown[] = "cudaErrorUnknown";
static const char string_unknown[] = "unknown error";

static const char unrecognized[] = "unrecognized error code";

#define get_error_impl(error, msg_type) \
    switch (error) { \
    case cudaSuccess: return msg_type ## _success; \
    case cudaErrorInsufficientDriver: return msg_type ## _insufficient_driver; \
    case cudaErrorInvalidValue: return msg_type ## _invalid; \
    case cudaErrorSystemDriverMismatch: return msg_type ## _driver_mismatch; \
    case cudaErrorUnknown: return msg_type ##_unknown; \
    default: return unrecognized; \
    }

DLL_PUBLIC
const char * cudaGetErrorName(cudaError_t error) {
    get_error_impl(error, name);
}

DLL_PUBLIC
const char * cudaGetErrorString(cudaError_t error) {
    get_error_impl(error, string);
}
