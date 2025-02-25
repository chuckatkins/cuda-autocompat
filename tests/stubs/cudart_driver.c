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
#include <dlfcn.h>

#include "cudart_error.h"
#include "visibility.h"

static void * driver_handle = NULL;
static int (*cuDriverGetVersion)(int *) = NULL;

int load_driver(void) {
    if (driver_handle != NULL) {
        return 1;
    }
    driver_handle = dlopen("libcuda.so.1", RTLD_NOW | RTLD_LOCAL);
    if (driver_handle == NULL) {
        return 2;
    }

    union {
        void *ptr;
        int (*fptr)(int *);
    } symbol;
    symbol.ptr = dlsym(driver_handle, "cuDriverGetVersion");
    if (symbol.ptr) {
        cuDriverGetVersion = symbol.fptr;
    }

    return 0;
}

DLL_DESTRUCTOR
void unload_driver(void) {
    if (driver_handle != NULL) {
        cuDriverGetVersion = NULL;
        dlclose(driver_handle);
        driver_handle = NULL;
    }
}

DLL_PUBLIC
cudaError_t cudaDriverGetVersion(int *ver) {
    if (ver == NULL) {
        return cudaErrorInvalidValue;
    }
    if (load_driver() != 0) {
        return cudaErrorInitializationError;
    }
    if (cuDriverGetVersion == NULL) {
        return cudaErrorSystemDriverMismatch;
    }
    return (cudaError_t)cuDriverGetVersion(ver);
}
