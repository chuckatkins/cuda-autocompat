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
#include <unistd.h>

#include <algorithm>
#include <format>
#include <string>

#include "logging.h"

namespace autocompat {

void init_logging(void) {
    LOGGING_MAX_LEVEL = log_level::warn;
    LOGGING_LOG_NAME = std::format("cuda_autocompat[{}]", ::getpid());

    constexpr int level_min = static_cast<int>(log_level::warn);
    constexpr int level_max = static_cast<int>(log_level::trace);
    const char *env_val = secure_getenv("CUDA_AUTOCOMPAT_VERBOSE");
    if (env_val != nullptr) {
        if (env_val[0] == '\0' || env_val[1] != '\0' || env_val[0] < '0' ||
            env_val[0] > '9') {
            log_warn("CUDA_AUTOCOMPAT_VERBOSE: Invalid value, using default {}",
                     static_cast<int>(LOGGING_MAX_LEVEL));
        } else {
            const int env_level = level_min + static_cast<int>(env_val[0] - '0');
            LOGGING_MAX_LEVEL =
                static_cast<log_level>(std::min(env_level, level_max));
        }
    }
}

} // namespace autocompat
