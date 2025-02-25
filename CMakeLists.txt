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

cmake_minimum_required(VERSION 3.25)

project(CUDAAutoCompat LANGUAGES C CXX VERSION 0.1.0)

list(PREPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(AutoCompatBuildUtils)

init_build_type()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Hide symbols not explicitly exposed as public
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_SHARED_LIBRARY_ENABLE_EXPORTS OFF)

# Always use PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Useful / necessary for linters and external analysis tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Reduce compiled binary size, especially when statically linking the
# C++ standard library
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_extra_flags()
add_static_cxx_flags()
add_security_flags()
add_coverage_flags()

include(CMakeDependentOption)
cmake_dependent_option(AUTOCOMPAT_ENABLE_EXAMPLES
    "Build libcuda and libcudart examples for testing"
    ON
    "NOT AUTOCOMPAT_ENABLE_COVERAGE"
    OFF
)

set(CPPCHECK_OPTIONS
    --check-level=exhaustive
)
set(INCLUDE_WHAT_YOU_USE_OPTIONS
    -Xiwyu --check_also=${PROJECT_SOURCE_DIR}/src/\*.h
    -Xiwyu --check_also=${PROJECT_BINARY_DIR}/src/\*.h
    -Xiwyu --mapping_file=${PROJECT_SOURCE_DIR}/.iwyu-mappings.imp
)
add_linter_options()

add_subdirectory(src)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
