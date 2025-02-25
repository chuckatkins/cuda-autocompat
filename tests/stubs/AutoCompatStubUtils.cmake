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

function(_add_stub_tree_target)
    if (NOT TARGET stub_tree)
        add_custom_target(stub_tree ALL)
    endif()
endfunction()

function(_add_stub_driver_impl)
    if (NOT TARGET stub_driver_impl)
        add_library(stub_driver_impl OBJECT
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cuda_error.h
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cuda_error.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cuda_init.c
        )
        target_link_libraries(stub_driver_impl PUBLIC
            utils_common
        )
    endif()
endfunction()

function(_add_stub_toolkit_impl)
    if (NOT TARGET stub_toolkit_impl)
        add_library(stub_toolkit_impl OBJECT
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cudart_error.h
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cudart_error.c
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/cudart_driver.c
        )
        target_link_libraries(stub_toolkit_impl PUBLIC
            utils_common
        )
    endif()
endfunction()

function(_add_stub_driver_extra_libs TARGET_NAME)
    get_target_property(TARGET_DIR ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        BYPRODUCTS
            ${TARGET_DIR}/libnvidia-nvvm.so.4
            ${TARGET_DIR}/libnvidia-ptxjitcompiler.so.1
            ${TARGET_DIR}/libcudadebugger.so.1
        COMMAND ${CMAKE_COMMAND} -E touch
            ${TARGET_DIR}/libnvidia-nvvm.so.4
            ${TARGET_DIR}/libnvidia-ptxjitcompiler.so.1
            ${TARGET_DIR}/libcudadebugger.so.1
    )
endfunction()

function(_add_stub_driver_symlinks TARGET_NAME)
    get_target_property(TARGET_DIR ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)
    cmake_path(GET TARGET_DIR FILENAME TARGET_DIRNAME)
    cmake_path(GET TARGET_DIR PARENT_PATH TARGET_BASE_DIR)

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMENT "Creating symlinks for ${TARGET_NAME}"
        WORKING_DIRECTORY ${TARGET_BASE_DIR}
        BYPRODUCTS ${TARGET_BASE_DIR}/${TARGET_DIRNAME}_symlink
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${TARGET_DIRNAME}
            ${TARGET_DIRNAME}_symlink
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGET_DIR}_symlinks
    )
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMENT "Creating symlinks for ${TARGET_NAME}"
        WORKING_DIRECTORY ${TARGET_DIR}_symlinks
        BYPRODUCTS
            ${TARGET_DIR}_symlinks/libcuda.so.1
            ${TARGET_DIR}_symlinks/libnvidia-nvvm.so.4
            ${TARGET_DIR}_symlinks/libnvidia-ptxjitcompiler.so.1
            ${TARGET_DIR}_symlinks/libcudadebugger.so.1
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ../${TARGET_DIRNAME}/libcuda.so.1
            libcuda.so.1
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ../${TARGET_DIRNAME}/libnvidia-nvvm.so.4
            libnvidia-nvvm.so.4
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ../${TARGET_DIRNAME}/libnvidia-ptxjitcompiler.so.1
            libnvidia-ptxjitcompiler.so.1
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ../${TARGET_DIRNAME}/libcudadebugger.so.1
            libcudadebugger.so.1
    )
endfunction()

function(_parse_cuda_ver version out_var)
    if (version MATCHES [=[([0-9]+)\.([0-9]+)\.([0-9])]=])
        math(EXPR ${out_var}
            "${CMAKE_MATCH_1}*1000+10*${CMAKE_MATCH_2}+${CMAKE_MATCH_3}"
        )
    else()
        set(${out_var} "${version}")
    endif()
    return(PROPAGATE ${out_var})
endfunction()

function(set_stub_driver_properties)
    set(options)
    set(oneValueArgs TARGET OUTPUT_DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if (NOT arg_TARGET)
        message(FATAL_ERROR "TARGET is empty or undefined")
    endif()
    if (NOT arg_OUTPUT_DIRECTORY)
        string(REGEX REPLACE [=[^stub_(.*)$]=] [=[\1]=] stub_name "${arg_TARGET}")
        set(arg_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/tests/stubs/tree/${stub_name}/lib
        )
    endif()

    set_target_properties(${arg_TARGET} PROPERTIES
        SOVERSION 1
        OUTPUT_NAME cuda
        LIBRARY_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
    )
    _add_stub_tree_target()
    add_dependencies(stub_tree ${arg_TARGET})
endfunction()

function(add_stub_driver)
    set(options NOIMPL NOLINKS)
    set(oneValueArgs TARGET VERSION)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if (NOT arg_TARGET)
        message(FATAL_ERROR "TARGET is empty or not defined")
    endif()
    if (NOT DEFINED arg_VERSION)
        message(FATAL_ERROR "VERSION is not defined")
    endif()

    _parse_cuda_ver(${arg_VERSION} c_version)
    add_library(${arg_TARGET} SHARED cuda_version.c)
    target_compile_definitions(${arg_TARGET} PRIVATE
        DRIVER_VERSION=${c_version}
    )
    target_link_libraries(${arg_TARGET} PRIVATE
        utils_common
    )
    if (NOT arg_NOIMPL)
        _add_stub_driver_impl()
        target_link_libraries(${arg_TARGET} PRIVATE stub_driver_impl)
    endif()
    set_target_properties(${arg_TARGET} PROPERTIES VERSION ${arg_VERSION})

    set_stub_driver_properties(TARGET ${arg_TARGET} ${arg_UNPARSED_ARGUMENTS})
    _add_stub_driver_extra_libs(${arg_TARGET})

    if (NOT arg_NOLINKS)
        _add_stub_driver_symlinks(${arg_TARGET})
    endif()
endfunction()

function(add_stub_toolkit)
    set(options NOCOMPAT)
    set(oneValueArgs TARGET VERSION)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if (NOT arg_TARGET)
        message(FATAL_ERROR "TARGET is empty or not defined")
    endif()
    if (NOT DEFINED arg_VERSION)
        message(FATAL_ERROR "VERSION is not defined")
    endif()

    _parse_cuda_ver(${arg_VERSION} c_version)
    add_library(${arg_TARGET} SHARED cudart_version.c)
    target_compile_definitions(${arg_TARGET} PRIVATE
        RUNTIME_VERSION=${c_version}
    )
    _add_stub_toolkit_impl()
    target_link_libraries(${arg_TARGET} PRIVATE
        utils_common
        stub_toolkit_impl
    )

    string(REGEX REPLACE [=[^stub_(.*)$]=] [=[\1]=] stub_name "${arg_TARGET}")
    set(toolkit_dir "${PROJECT_BINARY_DIR}/tests/stubs/tree/${stub_name}")
    set(toolkit_subdir "targets/${CMAKE_SYSTEM_PROCESSOR}-linux/lib")
    set(output_dir "${toolkit_dir}/${toolkit_subdir}")
    set_target_properties(${arg_TARGET} PROPERTIES
        VERSION ${arg_VERSION}
        SOVERSION 12
        OUTPUT_NAME cudart
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}
    )

    add_custom_command(TARGET ${arg_TARGET} POST_BUILD
        COMMENT "Adding symlink for ${arg_TARGET}"
        BYPRODUCTS ${toolkit_dir}/lib64
        WORKING_DIRECTORY ${toolkit_dir}
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${toolkit_subdir} lib64
    )

    if (NOT arg_NOCOMPAT)
        add_stub_driver(TARGET ${arg_TARGET}_compat
            VERSION ${arg_VERSION}
            OUTPUT_DIRECTORY ${toolkit_dir}/compat
            NOLINKS
        )
    endif()
    _add_stub_tree_target()
    add_dependencies(stub_tree ${arg_TARGET})
endfunction()

function(add_stub_other TARGET_NAME)
    string(REGEX REPLACE [=[^stub_(.*)$]=] [=[\1]=] stub_name "${TARGET_NAME}")
    set(output_base_dir "${PROJECT_BINARY_DIR}/tests/stubs/tree/${stub_name}")
    set(output_dir "${output_base_dir}/lib")

    add_custom_target(${TARGET_NAME}
        WORKING_DIRECTORY ${output_base_dir}
        BYPRODUCTS
            ${output_dir}/libfoo.so.0
            ${output_dir}/libbar.so.0
            ${output_dir}/libbaz.so.0
            ${output_base_dir}/lib_symlink
        COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
        COMMAND ${CMAKE_COMMAND} -E touch
            lib/libfoo.so.0
            lib/libbar.so.0
            lib/libbaz.so.0
        COMMAND ${CMAKE_COMMAND} -E create_symlink lib lib_symlink
    )
    _add_stub_tree_target()
    add_dependencies(stub_tree ${TARGET_NAME})
endfunction()
