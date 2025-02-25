# Copyright 2025 Chuck Atkins and CUDA Auto-Compat contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_subdirectory(utils)

# Core helper executable with the bulk of the search logic
add_executable(autocompat_search
    search/init.cxx
    search/parse_args.cxx
    search/search.cxx search/search.h
    search/main.cxx
)
target_link_libraries(autocompat_search
    PRIVATE
        extra_flags
        coverage_flags
        utils_common
        utils_version
        utils_cpp
)
set_target_properties(autocompat_search PROPERTIES
    OUTPUT_NAME cuda-autocompat-search
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBEXECDIR}
)

add_library(autocompat_audit SHARED rtld-audit/audit.c)
target_compile_definitions(autocompat_audit PRIVATE _GNU_SOURCE)
target_link_libraries(autocompat_audit
    PRIVATE
        extra_flags
        coverage_flags
        utils_common
        utils_version
        utils_c
)
set_target_properties(autocompat_audit PROPERTIES
    OUTPUT_NAME cuda_autocompat_audit
)

if (AUTOCOMPAT_ENABLE_EXAMPLES)
    add_executable(cuda_cuInit examples/cuda_cuInit.cxx)
    target_link_libraries(cuda_cuInit PRIVATE extra_flags utils_cpp)

    add_executable(cudart_cudaSetDevice examples/cudart_cudaSetDevice.cxx)
    target_link_libraries(cudart_cudaSetDevice PRIVATE extra_flags utils_cpp)
endif()
