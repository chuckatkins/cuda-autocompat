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

include(AutoCompatTestUtils)

add_subdirectory(stubs)

set(stub_tree_root ${CMAKE_CURRENT_BINARY_DIR}/stubs/tree)

add_autocompat_search_test(NAME single_driver_1
    PATHS ${stub_tree_root}/driver_123/lib
    ERROR_REGEX [=[ I libcuda: Updating \(first found\)]=]
)

add_autocompat_search_test(NAME single_driver_2
    PATHS ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ I libcuda: Updating \(first found\)]=]
)

add_autocompat_search_test(NAME single_driver_3
    PATHS ${stub_tree_root}/driver_567/lib
    ERROR_REGEX [=[ I libcuda: Updating \(first found\)]=]
)

add_autocompat_search_test(NAME single_driver_noerror
    PATHS ${stub_tree_root}/driver_noerror/lib
    ERROR_REGEX [=[ I libcuda: Skipping \(library error\)]=]
    WILL_FAIL
)

add_autocompat_search_test(NAME single_driver_autocompat
    PATHS ${stub_tree_root}/driver_autocompat/lib
    ERROR_REGEX [=[ I libcuda: Skipping \(autocompat detected\)]=]
    WILL_FAIL
)

add_autocompat_search_test(NAME single_toolkit
    PATHS ${stub_tree_root}/toolkit_345/lib64
    OUTPUT_REGEX ${stub_tree_root}/toolkit_345/compat
    ERROR_REGEX [=[ I libcuda: Updating \(first found\)]=]
)

add_autocompat_search_test(NAME single_toolkit_nocompat
    PATHS ${stub_tree_root}/toolkit_456/lib64
    WILL_FAIL
)

add_autocompat_search_test(NAME multipath
    PATHS
        ${stub_tree_root}/driver_123/lib
        ${stub_tree_root}/driver_234/lib
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ I libcuda: Updating \(2034 > 1023\)]=]
)

add_autocompat_search_test(NAME multipath_reversed
    PATHS
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_123/lib
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ I libcuda: Skipping \(1023 <= 2034\)]=]
)

add_autocompat_search_test(NAME cuda_home
    CUDA_HOME ${stub_tree_root}/toolkit_345
    PATHS ${stub_tree_root}/driver_123/lib
    OUTPUT_REGEX ${stub_tree_root}/toolkit_345/compat
    ERROR_REGEX [=[ I libcuda: Skipping \(1023 <= 3045\)]=]
)

add_autocompat_search_test(NAME driver_symlink_dir
    PATHS
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_234/lib_symlink
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ I libcuda: Skipping \(directory inode already checked\)]=]
)

add_autocompat_search_test(NAME driver_symlink_file
    PATHS
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_234/lib_symlinks
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ I libcuda: Skipping \(2034 <= 2034\)]=]
)

add_autocompat_search_test(NAME repeat_path
    PATHS
        ${stub_tree_root}/driver_123/lib
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_123/lib
    VERBOSE 3
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ D     skip .*/driver_123/lib \(already processed\)]=]
)

add_autocompat_search_test(NAME file_in_search_path
    PATHS
        ${stub_tree_root}/other_foo/lib/libfoo.so.0
        ${stub_tree_root}/driver_234/lib
    VERBOSE 3
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ D     skip .*/other_foo/lib/libfoo.so.0 \(not a directory\)]=]
)

add_autocompat_search_test(NAME dir_in_search_libs
    PATHS
        ${stub_tree_root}/driver_123/lib
    LIBRARIES
        ${stub_tree_root}/other_foo/lib
        ${stub_tree_root}/driver_234/lib/libcuda.so.1
    VERBOSE 3
    OUTPUT_REGEX ${stub_tree_root}/driver_234/lib
    ERROR_REGEX [=[ D     skip .*/other_foo/lib \(not a regular file\)]=]
)

add_autocompat_search_test(NAME loaded_driver
    LIBRARIES
        ${stub_tree_root}/other_foo/lib/libfoo.so.1
        ${stub_tree_root}/driver_123/lib/libcuda.so.1
        ${stub_tree_root}/other_bar/lib/libbar.so.1
        ${stub_tree_root}/other_baz/lib/libbaz.so.1
    PATHS
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_123/lib
    OUTPUT_REGEX ${stub_tree_root}/driver_123/lib
)

add_autocompat_search_test(NAME loaded_autocompat
    LIBRARIES
        ${stub_tree_root}/driver_autocompat/lib/libcuda.so.1
        ${stub_tree_root}/other_foo/lib/libfoo.so.1
        ${stub_tree_root}/driver_123/lib/libcuda.so.1
        ${stub_tree_root}/other_baz/lib/libbaz.so.1
    PATHS
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_123/lib
    OUTPUT_REGEX ${stub_tree_root}/driver_123/lib
)

add_autocompat_search_test(NAME loaded_toolkit
    LIBRARIES
        ${stub_tree_root}/other_foo/lib/libfoo.so.1
        ${stub_tree_root}/toolkit_345/lib64/libcudart.so.12
        ${stub_tree_root}/other_bar/lib/libbar.so.1
    PATHS
        ${stub_tree_root}/other_foo/lib
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/other_bar/lib
        ${stub_tree_root}/driver_123/lib
        ${stub_tree_root}/other_baz/lib
    OUTPUT_REGEX ${stub_tree_root}/toolkit_345/compat
)

add_autocompat_search_test(NAME full_search
    LIBRARIES
        ${stub_tree_root}/other_foo/lib                   # dir in libs
        ${stub_tree_root}/toolkit_345/lib/libcudart.so.12
        ${stub_tree_root}/other_bar/lib/libbar.so.1
    PATHS
        ${stub_tree_root}/driver_123/lib
        ${stub_tree_root}/driver_234/lib
        ${stub_tree_root}/driver_567/lib
        ${stub_tree_root}/driver_567/lib/libcuda.so.1     # file in path
        ${stub_tree_root}/driver_noerror/lib
        ""                                                # empty path
        ${stub_tree_root}/driver_autocompat/lib
        ${stub_tree_root}/driver_123/lib                  # repeat path
        ${stub_tree_root}/does_not_exist/lib              # DNE path
        ${stub_tree_root}/toolkit_456/lib64
        ${stub_tree_root}/driver_234_symlink/lib          # repeat driver symlink
    OUTPUT_REGEX ${stub_tree_root}/driver_567/lib
)

set(all_paths
    ${stub_tree_root}/driver_123/lib
    ${stub_tree_root}/driver_234/lib
    ${stub_tree_root}/driver_567/lib
    ${stub_tree_root}/driver_noerror/lib
    ${stub_tree_root}/driver_autocompat/lib
    ${stub_tree_root}/toolkit_345/lib64
    ${stub_tree_root}/toolkit_456/lib64
)
list(JOIN all_paths ":" all_paths)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/all_paths_args.txt "-p ${all_paths}")
add_autocompat_search_test(NAME stdin_args
    INPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/all_paths_args.txt
    OUTPUT_REGEX ${stub_tree_root}/driver_567/lib
)
