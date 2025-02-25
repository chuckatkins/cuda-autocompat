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

#include <limits.h>
#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "path_utils.h"
#include "search_helper.h"
#include "visibility.h"
#include "version.h"

#define LIBCUDA_SONAME "libcuda.so.1"
#define LIBNVIDIA_NVVM_SONAME "libnvidia-nvvm.so.4"
#define LIBNVIDIA_PTXJITCOMPILER_SONAME "libnvidia-ptxjitcompiler.so.1"
#define LIBCUDADEBUGGER_SONAME "libcudadebugger.so.1"

static char libcuda_path[PATH_MAX];
static char libnvidia_nvvm_path[PATH_MAX];
static char libnvidia_ptxjitcompiler_path[PATH_MAX];
static char libcudadebugger_path[PATH_MAX];
static int libcuda_path_len;
static int libnvidia_nvvm_path_len;
static int libnvidia_ptxjitcompiler_path_len;
static int libcudadebugger_path_len;

typedef struct {
    char data[PATH_MAX];
    char *slot;
    int slot_len;
    size_t trailing_len;
} ld_audit_backup;

void sanitize_ld_audit(ld_audit_backup *backup) {
    if (!backup) {
        fputs("warning: Cannot sanitize LD_AUDIT; invalid backup buffer\n",
              stderr);
        return;
    }
    if (backup->slot) {
        fputs("warning: Cannot sanitize LD_AUDIT; backup buffer is in use\n",
              stderr);
        return;
    }

    const char *ld_audit = secure_getenv("LD_AUDIT");
    if (!ld_audit) {
        backup->slot = NULL;
        return;
    }

    const char *self_path = get_path_to_self();
    if (!self_path) {
        backup->slot = NULL;
        fputs("warning: Cannot sanitize LD_AUDIT: failed to get path to self\n",
              stderr);
        return;
    }

    int self_fname_len;
    const char *self_fname = path_filename2(self_path, &self_fname_len);
    if (self_fname_len <= 0) {
        backup->slot = NULL;
        fputs("warning: Cannot sanitize LD_AUDIT: invalid filename for self\n",
              stderr);
        return;
    }

    // Walk through the colon-delimited LD_AUDIT looking for entries matching
    // the same filename as self
    const char *cursor = ld_audit;
    const char *token = NULL;
    int token_len = -1;
    while ((cursor = next_token(cursor, &token, &token_len, ':'))) {
        // token is smaller than the filename so don't bother checking it
        if (token_len < self_fname_len) {
            continue;
        }

        const char *token_fname = path_filename(token, token_len);
        int token_fname_len = token_len - (token_fname - token);
        if (token_fname_len == self_fname_len &&
            strncmp(token_fname, self_fname, self_fname_len) == 0) {
            // Backup the LD_AUDIT slot, including the trailing delimiter if
            // any
            int slot_len = cursor - token;
            backup->slot = (char *)token;
            backup->slot_len = slot_len;
            (void)memcpy(backup->data, backup->slot, slot_len);
            (void)memset(backup->data + slot_len, '\0', PATH_MAX - slot_len);

            // Replace the slot with the trailing data
            size_t trailing_len = strlen(cursor);
            backup->trailing_len = trailing_len;
            (void)memmove(backup->slot, cursor, trailing_len);
            (void)memset(backup->slot + trailing_len, '\0', slot_len);
            break;
        }
    }

    // No matching LD_AUDIT entry found so there's nothing to backup
    backup->slot = NULL;
}

void restore_ld_audit(ld_audit_backup *backup) {
    if (!backup) {
        fputs("warning: Cannot restore LD_AUDIT; invalid backup buffer\n",
              stderr);
        return;
    }
    if (!backup->slot) {
        fputs("warning: Cannot restore LD_AUDIT; backup buffer is not active\n",
              stderr);
        return;
    }

    // Shift the trailing data back to make room for the original slot and then
    // copy the backed up slot into it's original location
    (void)memmove(backup->slot + backup->slot_len, backup->slot,
                  backup->trailing_len);
    (void)memcpy(backup->slot, backup->data, backup->slot_len);

    // Clear the backup structre now that we're done with it
    (void)memset(backup, 0, sizeof(ld_audit_backup));
}

DLL_PUBLIC
unsigned int la_version(unsigned int version) {
    memset(libcuda_path, 0, PATH_MAX);
    memset(libnvidia_nvvm_path, 0, PATH_MAX);
    memset(libnvidia_ptxjitcompiler_path, 0, PATH_MAX);
    memset(libcudadebugger_path, 0, PATH_MAX);
    libcuda_path_len = -1;
    libnvidia_nvvm_path_len = -1;
    libnvidia_ptxjitcompiler_path_len = -1;
    libcudadebugger_path_len = -1;

    static ld_audit_backup backup;

    memset(&backup, 0, sizeof(backup));
    sanitize_ld_audit(&backup);

    static char libcuda_dir[PATH_MAX];
    size_t libcuda_dir_len = find_libcuda(libcuda_dir);
    if (libcuda_dir_len == 0) {
        fputs("error: Failed to locate a usable libcuda.so.1\n", stderr);
    } else {
        libcuda_path_len = path_join2(libcuda_path, libcuda_dir,
                                      libcuda_dir_len, LIBCUDA_SONAME);
        libnvidia_nvvm_path_len =
            path_join2(libnvidia_nvvm_path, libcuda_dir, libcuda_dir_len,
                       LIBNVIDIA_NVVM_SONAME);
        libnvidia_ptxjitcompiler_path_len =
            path_join2(libnvidia_ptxjitcompiler_path, libcuda_dir,
                       libcuda_dir_len, LIBNVIDIA_PTXJITCOMPILER_SONAME);
        libcudadebugger_path_len =
            path_join2(libcudadebugger_path, libcuda_dir, libcuda_dir_len,
                       LIBCUDADEBUGGER_SONAME);
    }

    if (backup.slot) {
        restore_ld_audit(&backup);
    }

    return LAV_CURRENT;
}

DLL_PUBLIC
char *la_objsearch(const char *name, uintptr_t *cookie, unsigned int flag) {
    // All lib paths will be set or none of them will be set so I only need to
    // check libcuda_path to know if initialization was successful
    if (libcuda_path[0] != '\0') {
        if (strcmp2(name, LIBCUDA_SONAME) == 0) {
            return libcuda_path;
        } else if (strcmp2(name, LIBNVIDIA_NVVM_SONAME) == 0) {
            return libnvidia_nvvm_path;
        } else if (strcmp2(name, LIBNVIDIA_PTXJITCOMPILER_SONAME) == 0) {
            return libnvidia_ptxjitcompiler_path;
        } else if (strcmp2(name, LIBCUDADEBUGGER_SONAME) == 0) {
            return libcudadebugger_path;
        }
    }

    return (char *)name;
}

DLL_PUBLIC
unsigned int la_objopen(struct link_map *map, Lmid_t lmid, uintptr_t *cookie) {
    const char *name = map->l_name;

    // Defensive null check (happens with main executable sometimes)
    if (!name || name[0] == '\0') {
        return 0;
    }

    if (strncmp(name, libcuda_path, libcuda_path_len) == 0 ||
        strncmp(name, libnvidia_nvvm_path, libnvidia_nvvm_path_len) == 0 ||
        strncmp(name, libnvidia_ptxjitcompiler_path,
                libnvidia_ptxjitcompiler_path_len) == 0 ||
        strncmp(name, libcudadebugger_path, libcudadebugger_path_len) == 0) {
        return LA_FLG_BINDTO | LA_FLG_BINDFROM;
    }

    return 0;
}
