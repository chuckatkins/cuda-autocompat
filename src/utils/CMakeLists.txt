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

add_library(utils_common INTERFACE
    common/visibility.h
)
target_include_directories(utils_common INTERFACE common)

configure_file(
    version/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version/version.h
    @ONLY
)
add_library(utils_version OBJECT
    version/version.c
    ${CMAKE_CURRENT_BINARY_DIR}/version/version.h
)
target_include_directories(utils_version PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/version
)
target_link_libraries(utils_version
    PRIVATE extra_flags coverage_flags
    PUBLIC utils_common
)

# C++ utilities
add_library(utils_cpp OBJECT
    cpp/dl_library.cxx cpp/dl_library.h
    cpp/logging.h
)
target_include_directories(utils_cpp PUBLIC cpp)
target_link_libraries(utils_cpp
    PRIVATE extra_flags coverage_flags
    PUBLIC utils_common
)

# C utilities
add_library(utils_c OBJECT
    c/path_utils.c c/path_utils.h
    c/search_helper.c c/search_helper.h
)
target_compile_definitions(utils_c PRIVATE _GNU_SOURCE)
target_include_directories(utils_c PUBLIC c)
target_link_libraries(utils_c
    PRIVATE
        extra_flags
        coverage_flags
        utils_common
        utils_version
)
