
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
#ifndef CUDA_AUTOCOMPAT_UTILS_C_PATH_UTILS_H
#define CUDA_AUTOCOMPAT_UTILS_C_PATH_UTILS_H

#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// This is a collection of helper utility functions for in-place heap-free path
// and string manipulations.  Several of the functions have a helper macro of
// the same name ending with 2 for some syntactic sugar to simplify their use
// with string literals instead of pointer+len.  For example:
//
// const char foo_path[] = "/foo/bar/lib64/libbaz.so.1";
// const char *end = path_prefix(foo_path, "lib", sizeof("lib") - 1, false);
// vs
// const char *end = path_prefix2(foo_path, "lib", false);
//
// or
//
// const char p[] = "/foo/bar/baz";
// size_t p_len = sizeof(parent);
// path_join(out_path, p, p_len, "baz", sizeof("baz") - 1);
// vs
// path_join2(out_path, p, p_len, "baz");

const char *get_path_to_self(void);

#define strlen2(str) (sizeof(str) - 1)
#define strcmp2(str1, str2) strncmp((str1), (str2), strlen2(str2))

// An in-place string tokenizer
//
// in:
//   cursor - A pointer to the beginning of a null-terminated string
//   delim  - A character delimiting the string
// out:
//   token     - A pointer the the start of the token (cursor on entrance)
//   token_len - The number of characters in the token excluding the
//               delimeter or trailing null termination; equivalent to strlen
//               if it were a null-terminated string
// return:
//   A pointer to the start of the next token; NULL on error
//
// Example:
// To iterate through the a colon separated PATH environment variable:
//
//   const char *path = secure_getenv("PATH");
//   const char *token = NULL
//   int token_len = 0;
//   while ((path = next_token(path, &token, &token_len)) {
//       printf("%.*s\n", token_len, token);
//   }
//
const char *next_token(const char *cursor, const char **token, int *token_len,
                       char delim);

// Determine the longest prefix in a given path containing a child directory
// with a given prefix
//
// in:
//   path             - A null-terminated string containg a file or directory
//                      path
//   child_prefix     - A string containing the prefix of the child directory
//   child_prefix_len - The number of characters in child_prefix
//   is_dir           - Whether path is a file path or directory path
// out:
//   remaining_len    - The length of the path after the prefix excluding
//                      the / delimiter
// return:
//   The length of the identified prefix; -1 on error
//
// Example:
//
//   const char foo[] = "/foo/bar/lib/x86_64-linux";
//   const char libfoo = "/foo/bar/lib64/libfoo.so";
//
//   path_prefix(foo, "lib", 3, true);
//   path_prefix(libfoo, "lib", 3, false);
//
// both return 8 for "/foo/bar"
//
int path_prefix(const char *path, const char *child_prefix,
                int child_prefix_len, bool is_dir);

#define path_prefix2(path, child, is_dir) \
    path_prefix((path), (child), strlen2(child), (is_dir))

// Join a parent and child path with a directory seperator and write the
// result to dst, padding the remaing buffer space with 0
//
int path_join(char dst[PATH_MAX], const char *parent, int parent_len,
              const char *child, int child_len);

#define path_join2(dst, parent_start, parent_len, child) \
    path_join((dst), (parent_start), (parent_len), (child), strlen2(child))

// Get the pointer to the start of the filename portion of a given path
const char *path_filename(const char *path, int path_len);

// Get the pointer to the start of the filename portion of a given path as a
// null-terminmated string
const char *path_filename2(const char *path, int *filename_len);

#endif // CUDA_AUTOCOMPAT_UTILS_C_PATH_UTILS_H
