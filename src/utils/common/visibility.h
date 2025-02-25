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

#ifndef CUDA_AUTOCOMPAT_TESTS_STUBS_VISIBILITY_H_
#define CUDA_AUTOCOMPAT_TESTS_STUBS_VISIBILITY_H_

#if defined(__GNUC__) || defined(__clang__)
    #define DLL_PUBLIC __attribute__((visibility("default")))
    #define DLL_PRIVATE __attribute__((visibility("hidden")))
    #define DLL_CONSTRUCTOR __attribute__((constructor))
    #define DLL_DESTRUCTOR __attribute__((destructor))
#else
    #define DLL_PUBLIC
    #define DLL_PRIVATE
    #define DLL_CONSTRUCTOR
    #define DLL_DESTRUCTOR
#endif

#endif // CUDA_AUTOCOMPAT_TESTS_STUBS_VISIBILITY_H_
