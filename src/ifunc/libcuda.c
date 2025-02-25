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

#include <stdlib.h>

#include <dlfcn.h>

#include "common/api.h"
#include "common/autocompat.h"
#include "common/logging.h"

void *libcuda_handle = NULL;
void *libnvidia_nvvm_handle = NULL;
void *libnvidia_ptxjitcompiler_handle = NULL;
void *libcudadebugger_handle = NULL;

int load_driver_libs(void) {
    LOG_INFO("Loading driver libs");
    int ret = 0;

    LOG_VERBOSE("%s", libcuda_path);
    LOG_TRACE("dlopen(%s)", libcuda_path);
    libcuda_handle = dlopen(libcuda_path, RTLD_LAZY | RTLD_GLOBAL);
    if (!libcuda_handle) {
        LOG_ERROR("Error loading %s: %s", LIBCUDA_SONAME, dlerror());
        ret = 1;
    }

    LOG_VERBOSE("%s", libnvidia_nvvm_path);
    LOG_TRACE("dlopen(%s)", libnvidia_nvvm_path);
    libnvidia_nvvm_handle =
        dlopen(libnvidia_nvvm_path, RTLD_LAZY | RTLD_GLOBAL);
    if (!libnvidia_nvvm_handle) {
        LOG_ERROR("Error loading %s: %s", LIBNVIDIA_NVVM_SONAME, dlerror());
        ret = 1;
    }

    LOG_VERBOSE("%s", libnvidia_ptxjitcompiler_path);
    LOG_TRACE("dlopen(%s)", libnvidia_ptxjitcompiler_path);
    libnvidia_ptxjitcompiler_handle =
        dlopen(libnvidia_ptxjitcompiler_path, RTLD_LAZY | RTLD_GLOBAL);
    if (!libnvidia_ptxjitcompiler_handle) {
        LOG_ERROR("Error loading %s: %s", LIBNVIDIA_PTXJITCOMPILER_SONAME,
                  dlerror());
        ret = 1;
    }

    /*
    libcudadebugger_handle = dlopen(libcuda_path, RTLD_LAZY | RTLD_GLOBAL);
    if (!libcudadebugger_handle) {
        LOG_ERROR("Error loading %s: %s", LIBCUDADEBUGGER_SONAME, dlerror());
        ret = 1;
    }
    */

    return ret;
}

int unload_driver_libs(void) {
    LOG_INFO("Unloading driver libs");
    int ret = 0;
    if (libcudadebugger_handle) {
        LOG_TRACE("dlclose(libcudadebugger)");
        if (dlclose(libcudadebugger_handle) != 0) {
            LOG_ERROR("Error closing %s: %s", LIBCUDADEBUGGER_SONAME,
                      dlerror());
            ret = 1;
        }
        libcudadebugger_handle = NULL;
    }
    if (libnvidia_ptxjitcompiler_handle) {
        LOG_TRACE("dlclose(libnvidia_ptxjitcompiler)");
        if (dlclose(libnvidia_ptxjitcompiler_handle) != 0) {
            LOG_ERROR("Error closing %s: %s", LIBNVIDIA_PTXJITCOMPILER_SONAME,
                      dlerror());
            ret = 1;
        }
        libnvidia_ptxjitcompiler_handle = NULL;
    }
    if (libnvidia_nvvm_handle) {
        LOG_TRACE("dlclose(libnvidia_nvvm)");
        if (dlclose(libnvidia_nvvm_handle) != 0) {
            LOG_ERROR("Error closing %s: %s", LIBNVIDIA_NVVM_SONAME, dlerror());
            ret = 1;
        }
        libnvidia_nvvm_handle = NULL;
    }
    if (libcuda_handle) {
        LOG_TRACE("dlclose(libcuda)");
        if (dlclose(libcuda_handle) != 0) {
            LOG_ERROR("Error closing %s: %s", LIBCUDA_SONAME, dlerror());
            ret = 1;
        }
        libcuda_handle = NULL;
    }
    return ret;
}

DLL_CONSTRUCTOR
void libcuda_ctor(void) {
    autocompat_init();

    LOG_INFO("CUDA AutoCompat v%d.%d.%d (libcuda IFUNC interface)",
             CUDA_AUTOCOMPAT_VERSION_MAJOR, CUDA_AUTOCOMPAT_VERSION_MINOR,
             CUDA_AUTOCOMPAT_VERSION_PATCH);

    int ret = autocompat_search();
    if (ret != 0) {
        if (ret == 1) {
            LOG_ERROR("No suitable %s found", LIBCUDA_SONAME);
        } else {
            LOG_ERROR("Search error");
        }
        exit(EXIT_FAILURE);
    }

    if (load_driver_libs() != 0) {
        LOG_ERROR("Error loading driver libs");
        if (unload_driver_libs() != 0) {
            LOG_ERROR("Error unloading driver libs");
        }
        exit(EXIT_FAILURE);
    }
}

DLL_DESTRUCTOR
void libcuda_dtor(void) {
    if (unload_driver_libs() != 0) {
        LOG_ERROR("Error unloading driver libs");
    }
    autocompat_destroy();
}
