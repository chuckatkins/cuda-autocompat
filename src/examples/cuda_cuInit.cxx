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

    log_info("Loading libcuda.so.1");
    const DlLibrary libcuda("libcuda.so.1");

#define check_dl_error(arg)                             \
    {                                                   \
        if (!(arg)) {                                     \
            log_info("  {}", libcuda.get_last_error()); \
            return EXIT_FAILURE;                        \
        }                                               \
    }

    check_dl_error(libcuda);
    log_info("  {}", libcuda.get_path());

    log_info("Looking up cuGetErrorName");
    auto cuGetErrorName =
        libcuda.get_function_symbol<int, int, const char *&>("cuGetErrorName");
    check_dl_error(cuGetErrorName);

    log_info("Looking up cuGetErrorString");
    auto cuGetErrorString =
        libcuda.get_function_symbol<int, int, const char *&>(
            "cuGetErrorString");
    check_dl_error(cuGetErrorString);

    log_info("Looking up cuDriverGetVersion");
    auto cuDriverGetVersion =
        libcuda.get_function_symbol<int, int *>("cuDriverGetVersion");
    check_dl_error(cuDriverGetVersion);

    log_info("Looking up cuInit");
    auto cuInit = libcuda.get_function_symbol<int, unsigned int>("cuInit");
    check_dl_error(cuInit);

    auto log_cuError = [&](const int ret) {
        const char *err_name = nullptr;
        const char *err_string = nullptr;
        if (cuGetErrorName(ret, err_name) == 0 &&
            cuGetErrorString(ret, err_string) == 0) {
            log_info("  ret = {} ({})", err_name, err_string);
        } else {
            log_info("  ret = {}", ret);
        }
    };

    int ret = 0;

    log_info("cuDriverGetVersion:");
    int ver = -1;
    ret = cuDriverGetVersion(&ver);
    if (ret == 0) {
        log_info("  ver = {}", ver);
    }
    log_cuError(ret);

    log_info("cuInit:");
    ret = cuInit(0);
    log_cuError(ret);

    return EXIT_SUCCESS;
}
