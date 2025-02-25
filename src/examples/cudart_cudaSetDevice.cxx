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

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "dl_library.h"
#include "logging.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    using namespace autocompat;

    LOGGING_MAX_LEVEL = log_level::info;
    LOGGING_USE_TIMESTAMP = false;
    LOGGING_USE_LOG_NAME = false;
    LOGGING_USE_LEVEL_NAME = false;

    log_info("Loading libcudart.so.12");
    const DlLibrary libcudart("libcudart.so.12");

#define check_dl_error(arg)                               \
    {                                                     \
        if (!(arg)) {                                     \
            log_info("  {}", libcudart.get_last_error()); \
            return EXIT_FAILURE;                          \
        }                                                 \
    }

    check_dl_error(libcudart);
    log_info("  {}", libcudart.get_path());

    log_info("Looking up cudaGetErrorName");
    auto cudaGetErrorName =
        libcudart.get_function_symbol<const char *, int>("cudaGetErrorName");
    check_dl_error(cudaGetErrorName);

    log_info("Looking up cudaGetErrorString");
    auto cudaGetErrorString =
        libcudart.get_function_symbol<const char *, int>("cudaGetErrorString");
    check_dl_error(cudaGetErrorString);

    log_info("Looking up cudaDriverGetVersion");
    auto cudaDriverGetVersion =
        libcudart.get_function_symbol<int, int &>("cudaDriverGetVersion");
    check_dl_error(cudaDriverGetVersion);

    log_info("Looking up cudaRuntimeGetVersion");
    auto cudaRuntimeGetVersion =
        libcudart.get_function_symbol<int, int &>("cudaRuntimeGetVersion");
    check_dl_error(cudaRuntimeGetVersion);

    log_info("Looking up cudaSetDevice");
    auto cudaSetDevice =
        libcudart.get_function_symbol<int, int>("cudaSetDevice");
    check_dl_error(cudaSetDevice);

    auto log_cudaError = [&](const int ret) {
        const char *err_name = cudaGetErrorName(ret);
        const char *err_string = cudaGetErrorString(ret);
        if (err_name != nullptr && err_string != nullptr) {
            log_info("  ret = {} ({})", err_name, err_string);
        } else {
            log_info("  ret = {}", ret);
        }
    };

    int ret = 0;
    int ver = -1;

    log_info("cudaDriverGetVersion:");
    ret = cudaDriverGetVersion(ver);
    if (ret == 0) {
        log_info("  ver = {}", ver);
    }
    log_cudaError(ret);

    log_info("cudaRuntimeGetVersion:");
    ret = cudaRuntimeGetVersion(ver);
    if (ret == 0) {
        log_info("  ver = {}", ver);
    }
    log_cudaError(ret);

    log_info("cudaSetDevice:");
    ret = cudaSetDevice(0);
    log_cudaError(ret);

    return EXIT_SUCCESS;
}
