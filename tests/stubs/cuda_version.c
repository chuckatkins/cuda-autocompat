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

#include <stddef.h>

#include "cuda_error.h"
#include "visibility.h"

#ifndef DRIVER_VERSION
    #error "DRIVER_VERSION must be defined"
#endif

static const int driver_version = DRIVER_VERSION;

DLL_PUBLIC
CUresult cuDriverGetVersion(int *ver) {
    if (ver == NULL) {
        return CUDA_ERROR_INVALID_VALUE;
    }

    *ver = driver_version;
    return CUDA_SUCCESS;
}
