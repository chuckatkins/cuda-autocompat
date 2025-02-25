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

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "path_utils.h"

#define HELPER_EXE "cuda-autocompat-search"

bool find_search_helper(char out_path[PATH_MAX]) {
    const char *self_path = get_path_to_self();
    if (!self_path) {
        fputs("error: Failed to get path to self\n", stderr);
        return false;
    }

    int prefix_len = path_prefix2(self_path, "lib", false);
    if (prefix_len == -1) {
        memset(out_path, 0, PATH_MAX);
        (void)fputs("error: Unable to determine prefix for self.\n", stderr);
        return false;
    }

    if (path_join2(out_path, self_path, prefix_len, "/libexec/" HELPER_EXE) ==
        -1) {
        memset(out_path, 0, PATH_MAX);
        (void)fputs("error: Sibling path truncated\n", stderr);
        return false;
    }
    if (access(out_path, R_OK | X_OK) == 0) {
        return true;
    }

    // Couldn't find it in libexec so we need to search PATH
    const char *path = secure_getenv("PATH");
    if (!path) {
        memset(out_path, 0, PATH_MAX);
        (void)fputs("Unable to read PATH from environment\n", stderr);
        return false;
    }

    const char *p_start = NULL;
    int p_len = 0;
    while ((path = next_token(path, &p_start, &p_len, ':'))) {
        if (path_join2(out_path, p_start, p_len, HELPER_EXE) == -1) {
            if (access(out_path, R_OK | X_OK) == 0) {
                return true;
            }
        } else {
            (void)fputs("warning: Path truncated; skipping\n", stderr);
        }
    }

    memset(out_path, 0, PATH_MAX);
    return false;
}

size_t find_libcuda(char out_path[PATH_MAX]) {
    memset(out_path, 0, PATH_MAX);

    char search_helper_path[PATH_MAX];
    if (!find_search_helper(search_helper_path)) {
        (void)fputs("error: Failed to locate cuda-autocompat-search helper\n",
                    stderr);
        return 0;
    }

    FILE *pipe = popen(search_helper_path, "r");
    if (!pipe) {
        (void)fputs("error: Failed to execute search helper\n", stderr);
        return 0;
    }

    char *buf_cur = out_path;
    size_t bytes_read;
    size_t bytes_remaining = PATH_MAX - 1;
    while (bytes_remaining > 0 &&
           (bytes_read = fread(buf_cur, 1, bytes_remaining, pipe)) > 0) {
        // Validate we have clean input and cut out early if we don't
        void *eod;
        if ((eod = memchr(buf_cur, '\0', bytes_read)) ||
            (eod = memchr(buf_cur, '\n', bytes_read))) {
            size_t eod_offset = (size_t)((char *)eod - buf_cur);
            memset(eod, 0, bytes_read - eod_offset);
            buf_cur = (char *)eod;
            break;
        }

        bytes_remaining -= bytes_read;
        buf_cur += bytes_read;
    }

    int ret = pclose(pipe);
    if (ret == -1) {
        fputs("error: ", stderr);
        fputs(strerror(errno), stderr);
        fputc('\n', stderr);
        memset(out_path, 0, PATH_MAX);
        return 0;
    } else if (ret != 0) {
        fputs("error: Search helper failed\n", stderr);
        memset(out_path, 0, PATH_MAX);
        return 0;
    }

    return (size_t)(buf_cur - out_path);
}

