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

function(add_wrapped_test)
    set(options OUTPUT_QUIET ERROR_QUIET WILL_FAIL)
    set(oneValueArgs NAME INPUT_FILE)
    set(multiValueArgs ENVIRONMENT COMMAND OUTPUT_REGEX ERROR_REGEX)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if (NOT arg_NAME)
        message(FATAL_ERROR "NAME is empty or not set")
    endif()
    if (NOT arg_COMMAND)
        message(FATAL_ERROR "COMMAND is empty or not set")
    endif()
    if (arg_OUTPUT_QUIET AND arg_OUTPUT_REGEX)
        message(FATAL_ERROR "OUTPUT_QUIET and OUTPUT_REGEX cannot both be set")
    endif()
    if (arg_ERROR_QUIET AND arg_ERROR_REGEX)
        message(FATAL_ERROR "ERROR_QUIET and ERROR_REGEX cannot both be set")
    endif()

    set(exec_args -DLIST_SEPARATOR=,)
    if (arg_WILL_FAIL)
        list(APPEND exec_args -DWILL_FAIL=TRUE)
    endif()
    if (arg_INPUT_FILE)
        list(APPEND exec_args -DINPUT_FILE=${arg_INPUT_FILE})
    endif()
    if (arg_OUTPUT_QUIET)
        list(APPEND exec_args -DOUTPUT_QUIET=TRUE)
    elseif (arg_OUTPUT_REGEX)
        list(APPEND exec_args -DOUTPUT_REGEX=${arg_OUTPUT_REGEX})
    endif()
    if (arg_ERROR_QUIET)
        list(APPEND exec_args -DERROR_QUIET=TRUE)
    elseif (arg_ERROR_REGEX)
        list(APPEND exec_args -DERROR_REGEX=${arg_ERROR_REGEX})
    endif()
    if (arg_ENVIRONMENT)
        list(JOIN arg_ENVIRONMENT "," arg_ENVIRONMENT)
        list(APPEND exec_args -DENVIRONMENT=${arg_ENVIRONMENT})
    endif()
    list(JOIN arg_COMMAND "," arg_COMMAND)
    list(APPEND exec_args -DEXE=${arg_COMMAND})

    add_test(NAME ${arg_NAME}
        COMMAND ${CMAKE_COMMAND}
            ${exec_args}
            -P ${PROJECT_SOURCE_DIR}/cmake/AutoCompatExecWrapper.cmake
    )

    if (AUTOCOMPAT_ENABLE_COVERAGE)
        set_tests_properties(${arg_NAME} PROPERTIES
            FIXTURES_REQUIRED COVERAGE
        )
    endif()
endfunction()

function(add_autocompat_search_test)
    set(options WILL_FAIL)
    set(oneValueArgs NAME INPUT_FILE CUDA_HOME VERBOSE)
    set(multiValueArgs PATHS LIBRARIES OUTPUT_REGEX ERROR_REGEX)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    if (NOT arg_NAME)
        message(FATAL_ERROR "NAME is empty or not set")
    endif()

    set(wrapped_args)
    if (arg_WILL_FAIL)
        list(APPEND wrapped_args WILL_FAIL TRUE)
    endif()

    if (arg_OUTPUT_REGEX)
        list(APPEND wrapped_args OUTPUT_REGEX ${OUTPUT_REGEX})
    elseif (NOT arg_WILL_FAIL)
        # Try to infer the pasing output path
        if (arg_PATHS AND NOT arg_LIBRARIES)
            list(LENGTH arg_PATHS paths_len)
            if (paths_len EQUAL 1)
                list(APPEND wrapped_args OUTPUT_REGEX ${arg_PATHS})
            endif()
        elseif (arg_LIBRARIES AND NOT arg_PATHS)
            list(LENGTH arg_LIBRARIES libs_len)
            if (libs_len EQUAL 1)
                cmake_path(GET arg_LIBRARIES PARENT_PATH pass_path)
                list(APPEND wrapped_args OUTPUT_REGEX ${pass_path})
            endif()
        endif()
    endif()

    if (arg_ERROR_REGEX)
        list(APPEND wrapped_args ERROR_REGEX "${arg_ERROR_REGEX}")
    endif()

    if (arg_INPUT_FILE)
        list(APPEND wrapped_args INPUT_FILE ${arg_INPUT_FILE})
    endif()

    set(env)
    if (arg_CUDA_HOME)
        list(APPEND env CUDA_HOME=${arg_CUDA_HOME})
    else()
        list(APPEND env CUDA_HOME=)
    endif()
    if (arg_VERBOSE)
        list(APPEND env CUDA_AUTOCOMPAT_VERBOSE=${arg_VERBOSE})
    else()
        list(APPEND env CUDA_AUTOCOMPAT_VERBOSE=2)
    endif()

    list(APPEND wrapped_args ENVIRONMENT "${env}")

    set(exe $<TARGET_FILE:autocompat_search>)
    if (arg_PATHS)
        list(JOIN arg_PATHS ":" arg_PATHS)
        list(APPEND exe -p "${arg_PATHS}")
    endif()
    if (arg_LIBRARIES)
        list(JOIN arg_LIBRARIES ":" arg_LIBRARIES)
        list(APPEND exe -l "${arg_LIBRARIES}")
    endif()
    if (arg_INPUT_FILE)
        list(APPEND exe -)
    endif()

    add_wrapped_test(NAME ${arg_NAME}
        COMMAND ${exe}
        ${wrapped_args}
    )
endfunction()
