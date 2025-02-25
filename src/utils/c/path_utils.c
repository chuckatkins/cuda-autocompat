
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

#include "path_utils.h"

#include <dlfcn.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "version.h"

const char *get_path_to_self(void) {
    static const char *self_path = NULL;
    static Dl_info self_info;

    if (!self_path) {
        if (dladdr((const void *)&cuda_autocompat_version, &self_info) != 0) {
            self_path = self_info.dli_fname;
        }
    }
    return self_path;
}

const char *next_token(const char *cursor, const char **token, int *token_len,
                       char delim) {
    if (!cursor || !token || !token_len || *cursor == '\0') {
        return NULL;
    }

    *token = cursor;
    const char *token_end = *token;
    while (*token_end != delim && *token_end != '\0') {
        ++token_end;
    }

    *token_len = token_end - *token;
    return (*token_end == delim) ? token_end + 1 : token_end;
}

inline int path_prefix(const char *path, const char *child_prefix,
                       int child_prefix_len, bool is_dir) {
    if (!path || !child_prefix || child_prefix_len <= 0) {
        return -1;
    }

    const char *cursor = path;
    const char *match = NULL;
    const char *component = NULL;
    int component_len = 0;

    while ((cursor = next_token(cursor, &component, &component_len, '/'))) {
        // Ignore the last filename component
        if (!is_dir && component[component_len] == '\0') {
            break;
        }

        if (component_len >= child_prefix_len &&
            memcmp(component, child_prefix, child_prefix_len) == 0) {
            match = component - 1;
        }
    }

    return match ? match - path : -1;
}

inline int path_join(char dst[PATH_MAX], const char *parent, int parent_len,
                     const char *child, int child_len) {
    // If parent is "", treat as "."
    static const char dot[] = ".";
    if (parent_len <= 0 || (parent_len == 1 && parent[0] == '\0')) {
        parent = dot;
        parent_len = 1;
    }

    bool parent_has_sep = parent_len > 0 && parent[parent_len - 1] == '/';
    bool child_has_sep = child_len > 0 && child[0] == '/';

    int extra_sep = (parent_has_sep || child_has_sep) ? 0 : 1;

    // Check for overflow (+1 null terminator already included in clear)
    int total_len = parent_len + child_len + extra_sep;
    if (total_len >= PATH_MAX) {
        return -1;
    }

    char *cursor = mempcpy(dst, parent, parent_len);

    if (extra_sep) {
        *cursor++ = '/';
        if (child_has_sep) {
            ++child;
            --child_len;
        }
    }

    cursor = mempcpy(cursor, child, child_len);

    // Zero out the rest of the buffer
    memset(cursor, 0, PATH_MAX - (size_t)(cursor - dst));
    return total_len;
}

const char *path_filename(const char *path, int path_len) {
    if (!path) {
        return NULL;
    }

    const char *last_dir = NULL;
    const char *cur;
    const char *end = path + path_len;
    for (cur = path; cur < end; ++cur) {
        if (*cur == '/') {
            last_dir = cur;
        }
    }

    return last_dir ? last_dir + 1 : path;
}

const char *path_filename2(const char *path, int *filename_len) {
    if (!path) {
        return NULL;
    }

    const char *last_dir = NULL;
    const char *cur;
    for (cur = path; *cur != '\0'; ++cur) {
        if (*cur == '/') {
            last_dir = cur;
        }
    }

    const char *filename = last_dir ? last_dir + 1 : path;
    if (filename_len) {
        *filename_len = cur - filename;
    }
    return filename;
}
