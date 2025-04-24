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

macro(init_build_type)
    if (NOT DEFINED CMAKE_CONFIGURATION_TYPES)
        if (NOT CMAKE_BUILD_TYPE)
            set(CMAKE_BUILD_TYPE Debug)
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE Debug)
        endif()
    endif()
endmacro()

function(check_ci_env)
    if (NOT "$ENV{CI}" STREQUAL "true")
        return()
    endif()

    separate_arguments(cflags NATIVE_COMMAND "$ENV{CFLAGS}")
    separate_arguments(cxxflags NATIVE_COMMAND "$ENV{CXXFLAGS}")
    separate_arguments(ldflags NATIVE_COMMAND "$ENV{LDFLAGS}")
    set(extra_cflags)
    set(extra_cxxflags)
    set(extra_ldflags)

    if ("$ENV{CI_COMPILER}" MATCHES [[^gcc(.*)$]])
        set(ENV{CC} "${CMAKE_MATCH_0}")
        set(ENV{CXX} "g++${CMAKE_MATCH_1}")
        message(STATUS "CI: CC=$ENV{CC} CXX=$ENV{CXX}")
        if (NOT "$ENV{CI_STDLIB}" MATCHES [[^(libstdc\+\+)?$]])
            message(WARNING "Ignoring CI_STDLIB=$ENV{CI_STDLIB}")
        endif()
        if (NOT "$ENV{CI_RTLIB}" MATCHES [[^(libgcc)?$]])
            message(WARNING "Ignoring CI_RTLIB=$ENV{CI_RTLIB}")
        endif()
        if ("$ENV{CI_LINKER}" STREQUAL lld)
            list(APPEND extra_ldflags -fuse-ld=$ENV{CI_LINKER})
        elseif (NOT "$ENV{CI_LINKER}" STREQUAL ld)
            message(WARNING "Ignoring CI_LINKER=$ENV{CI_LINKER}")
        endif()
    elseif ("$ENV{CI_COMPILER}" MATCHES [[^clang(.*)$]])
        set(ENV{CC} "${CMAKE_MATCH_0}")
        set(ENV{CXX} "clang++${CMAKE_MATCH_1}")
        message(STATUS "CI: CC=$ENV{CC} CXX=$ENV{CXX}")

        if (DEFINED ENV{CI_STDLIB})
            if ("$ENV{CI_STDLIB}" MATCHES [[^lib(std)?c\+\+$]])
                list(APPEND extra_cxxflags -stdlib=$ENV{CI_STDLIB})
            else()
                message(WARNING "Ignoring CI_STDLIB=$ENV{CI_STDLIB}")
            endif()
        endif()

        if (DEFINED ENV{CI_RTLIB})
            if ("$ENV{CI_RTLIB}" STREQUAL libgcc)
                list(APPEND extra_ldflags -rtlib=libgcc -unwindlib=libgcc)
            elseif("$ENV{CI_RTLIB}" STREQUAL compiler-rt)
                list(APPEND extra_ldflags -rtlib=compiler-rt -unwindlib=libunwind)
            else()
                message(WARNING "Ignoring CI_RTLIB=$ENV{CI_RTLIB}")
            endif()
        endif()

        if (DEFINED ENV{CI_LINKER})
            list(APPEND extra_ldflags -fuse-ld=$ENV{CI_LINKER})
        endif()
    elseif (DEFINED ENV{CI_COMPILER})
        message(WARNING "Ignoring CI_COMPILER=$ENV{CI_COMPILER}")
        if (DEFINED ENV{CI_STDLIB})
            message(WARNING "Ignoring CI_STDLIB=$ENV{CI_STDLIB}")
        endif()
        if (DEFINED ENV{CI_RTLIB})
            message(WARNING "Ignoring CI_RTLIB=$ENV{CI_RTLIB}")
        endif()
        if (DEFINED ENV{CI_LINKER})
            message(WARNING "Ignoring CI_LINKER=$ENV{CI_LINKER}")
        endif()
    endif()

    if (extra_cflags)
        list(APPEND cflags ${extra_cflags})
        list(JOIN cflags " " cflags)
        set(ENV{CFLAGS} "${cflags}")
        list(JOIN extra_cflags " " extra_cflags)
        message(STATUS "CI: CFLAGS=\"${extra_cflags}\"")
    endif()
    if (extra_cxxflags)
        list(APPEND cxxflags ${extra_cxxflags})
        list(JOIN cxxflags " " cxxflags)
        set(ENV{CXXFLAGS} "${cxxflags}")
        list(JOIN extra_cxxflags " " extra_cxxflags)
        message(STATUS "CI: CXXFLAGS=\"${extra_cxxflags}\"")
    endif()
    if (extra_ldflags)
        list(APPEND ldflags ${extra_ldflags})
        list(JOIN ldflags " " ldflags)
        set(ENV{LDFLAGS} "${ldflags}")
        list(JOIN extra_ldflags " " extra_ldflags)
        message(STATUS "CI: LDFLAGS=\"${extra_ldflags}\"")
    endif()
endfunction()

function(_check_stdcxx_flavor)
    include(CheckCXXSymbolExists)
    check_cxx_symbol_exists(__GLIBCXX__ "version" _STDCXX_FLAVOR_GNU)
    check_cxx_symbol_exists(_LIBCPP_VERSION "version" _STDCXX_FLAVOR_LLVM)
endfunction()

function(_add_extra_flags_target)
    if (NOT TARGET extra_flags)
        add_library(extra_flags INTERFACE)
    endif()
endfunction()

function(_add_supported_compile_flags)
    _add_extra_flags_target()

    include(CheckCompilerFlag)

    get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
    foreach(LANG IN LISTS ENABLED_LANGUAGES)
        set(${LANG}_compile_flags)
    endforeach()

    set(CMAKE_REQUIRED_QUIET TRUE)
    foreach(flag IN LISTS ARGN)
        string(MAKE_C_IDENTIFIER "${flag}" flag_var)
        if (NOT DEFINED CACHE{_FLAG_CHECKED_${flag_var}})
            message(CHECK_START "Checking flag ${flag}")
        endif()

        set(flag_langs)
        foreach(LANG IN LISTS ENABLED_LANGUAGES)
            set(flag_compile_var "_FLAG_${LANG}_COMPILE_${flag_var}")
            check_compiler_flag(${LANG} "${flag}" ${flag_compile_var})
            if (${flag_compile_var})
                list(APPEND flag_langs ${LANG})
            endif()
        endforeach()
        list(JOIN flag_langs "," flag_langs)
        target_compile_options(extra_flags INTERFACE
            $<$<COMPILE_LANGUAGE:${flag_langs}>:${flag}>
        )

        if (NOT DEFINED _FLAG_CHECKED_${flag_var})
            if (flag_langs)
                message(CHECK_PASS "${flag_langs}")
            else()
                message(CHECK_FAIL "failed")
            endif()
            set(_FLAG_CHECKED_${flag_var} 1 CACHE INTERNAL "${flag}")
        endif()
    endforeach()
endfunction()

function(_add_safestack_if_supported)
    include(CheckCSourceCompiles)

    set(CMAKE_REQUIRED_FLAGS -fsanitize=safe-stack)
    set(CMAKE_REQUIRED_LINK_OPTIONS -fsanitize=safe-stack)
    check_c_source_compiles("
        int main(void) {
            volatile int x = 42;
            return x;
        }
        " HAS_SAFE_STACK
    )
    if (HAS_SAFE_STACK)
        target_compile_options(extra_flags INTERFACE -fsanitize=safe-stack)
        target_link_options(extra_flags INTERFACE -fsanitize=safe-stack)
    endif()
endfunction()


function(add_extra_flags)
    _add_extra_flags_target()

    # Enable all the warnings
    _add_supported_compile_flags(
        -Werror
        -Wall
        -Wpedantic
    )

    # Remove unused sections
    _add_supported_compile_flags(
        -ffunction-sections
        -fdata-sections
    )
    target_link_options(extra_flags INTERFACE -Wl,--gc-sections)

    if (CMAKE_CXX_COMPILER_LINKER_ID STREQUAL LLD)
         target_link_options(extra_flags INTERFACE -Wl,--icf=all)
    endif()
endfunction()

function(add_static_cxx_flags)
    _add_extra_flags_target()

    option(AUTOCOMPAT_ENABLE_STATIC_STDCXX
        "Statically link the C++ standard library"
        ON
    )
    if (NOT AUTOCOMPAT_ENABLE_STATIC_STDCXX)
        return()
    endif()

    _check_stdcxx_flavor()

    _add_supported_compile_flags(-fno-exceptions)

    # Note that -static-libstdc++ works for both GCC and Clang.  Even when
    # using -stdlib=libc++ with Clang, -static-libstdc++ will link libc++.a
    # despite the implication of the option name.
    target_link_options(extra_flags INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:-static-libstdc++>
    )
endfunction()

function(add_security_flags)
    _add_extra_flags_target()

    option(AUTOCOMPAT_ENABLE_SECURITY_FLAGS
        "Add additional security related compiler flags"
        ON
    )
    if (NOT AUTOCOMPAT_ENABLE_SECURITY_FLAGS)
        return()
    endif()

    _check_stdcxx_flavor()

    # Taken from the following sources:
    #
    # "Use compiler flags for stack protection in GCC and Clang"
    # Red Hat
    # https://developers.redhat.com/articles/2022/06/02/use-compiler-flags-stack-protection-gcc-and-clang
    #
    # "Compiler Options Hardening Guide for C and C++"
    # Open Source Security Foundation (OpenSSF) Best Practices Working Group
    # https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
    # 

    target_compile_definitions(extra_flags INTERFACE
        $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=3>
    )
    if (_STDCXX_FLAVOR_GNU)
        target_compile_definitions(extra_flags INTERFACE
            $<$<COMPILE_LANGUAGE:CXX>:_GLIBCXX_ASSERTIONS>
        )
    elseif(_STDCXX_FLAVOR_LLVM)
        target_compile_definitions(extra_flags INTERFACE
            $<$<COMPILE_LANGUAGE:CXX>:_LIBCPP_ENABLE_HARDENED_MODE>
        )
    endif()

    _add_supported_compile_flags(
        ${out_compile_var}
        -fhardened
        -Wno-hardened
        -fstack-protector-all
        -fstack-clash-protection
        -fstrict-flex-arrays=3
        -fno-delete-null-pointer-checks
    )
    target_link_options(extra_flags INTERFACE
        -Wl,-z,noexecstack
        -Wl,-z,relro
        -Wl,-z,now
        -Wl,--as-needed
        -Wl,--no-copy-dt-needed-entries
    )
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
        _add_supported_compile_flags(-fcf-protection=full)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
        _add_supported_compile_flags(-mbranch-protection=standard)
    endif()

    #_add_safestack_if_supported()
endfunction()

function(_add_linter_option LINTER_NAME)
    if (AUTOCOMPAT_ENABLE_LINTERS)
        string(TOLOWER "${LINTER_NAME}" LINTER_COMMAND)
        string(REPLACE "_" "-" LINTER_COMMAND "${LINTER_COMMAND}")
        message(CHECK_START "Looking for ${LINTER_COMMAND}")
        find_program(${LINTER_NAME}_EXECUTABLE ${LINTER_COMMAND})
        mark_as_advanced(${LINTER_NAME}_EXECUTABLE)
        if (${LINTER_NAME}_EXECUTABLE)
            message(CHECK_PASS "${${LINTER_NAME}_EXECUTABLE}")
            list(APPEND AUTOCOMPAT_LINTERS_FOUND ${LINTER_NAME})
        else()
            message(CHECK_FAIL "${${LINTER_NAME}_EXECUTABLE}")
        endif()
    endif()

    get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
    foreach(LANG IN LISTS ENABLED_LANGUAGES)
        if (AUTOCOMPAT_ENABLE_LINTERS AND
            ${LINTER_NAME} IN_LIST AUTOCOMPAT_LINTERS_FOUND)
            set(CMAKE_${LANG}_${LINTER_NAME}
                ${${LINTER_NAME}_EXECUTABLE} ${${LINTER_NAME}_OPTIONS}
                CACHE PATH "")
        else()
            unset(CMAKE_${LANG}_${LINTER_NAME} CACHE)
        endif()
    endforeach()

    return(PROPAGATE AUTOCOMPAT_LINTERS_FOUND)
endfunction()

function(add_linter_options)
    option(AUTOCOMPAT_ENABLE_LINTERS
        "Enable cppcheck, iwyu, and clang-tidy"
        OFF
    )

    unset(AUTOCOMPAT_LINTERS_FOUND)

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES Clang)
        set(CLANG_TIDY_EXECUTABLE IGNORE CACHE FILEPATH "")
        set(INCLUDE_WHAT_YOU_USE_EXECUTABLE IGNORE CACHE FILEPATH "")
    endif()

    _add_linter_option(CPPCHECK)
    _add_linter_option(INCLUDE_WHAT_YOU_USE)
    _add_linter_option(CLANG_TIDY)

    if (AUTOCOMPAT_ENABLE_LINTERS)
        if (NOT AUTOCOMPAT_LINTERS_FOUND)
            message(FATAL_ERROR "Linting is enabled but no linters were found")
        elseif (AUTOCOMPAT_LINTERS_FOUND)
            string(REPLACE ";" " " _AUTOCOMPAT_LINTERS_FOUND_SPACE
                "${AUTOCOMPAT_LINTERS_FOUND}"
            )
            message(STATUS "Linters enabled: ${_AUTOCOMPAT_LINTERS_FOUND_SPACE}")
        endif()
    endif()
    return(PROPAGATE AUTOCOMPAT_ENABLE_LINTERS AUTOCOMPAT_LINTERS_FOUND)
endfunction()

function(add_coverage_flags)
    if (NOT TARGET coverage_flags)
        add_library(coverage_flags INTERFACE)
    endif()

    include(CTest)
    include(CMakeDependentOption)
    cmake_dependent_option(AUTOCOMPAT_ENABLE_COVERAGE
        "Enable gcov based code coverage reports"
        OFF
        "AUTOCOMPAT_ENABLE_TESTING"
        OFF
    )
    if (NOT AUTOCOMPAT_ENABLE_COVERAGE)
        return()
    endif()

    find_program(LCOV_EXECUTABLE lcov REQUIRED)
    mark_as_advanced(LCOV_EXECUTABLE)
    find_program(GENHTML_EXECUTABLE genhtml REQUIRED)
    mark_as_advanced(GENHTML_EXECUTABLE)

    set(exclude_dirs
        ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES}
        ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
        ${PROJECT_SOURCE_DIR}/src/examples
    )
    set(LCOV_EXCLUDES)
    list(REMOVE_DUPLICATES exclude_dirs)
    foreach (dir IN LISTS exclude_dirs)
        list(APPEND LCOV_EXCLUDES --exclude ${dir}/*)
    endforeach()

    target_compile_options(coverage_flags INTERFACE -coverage)
    target_link_options(coverage_flags INTERFACE -coverage)

    # Setup test fixtures to generate code coverage reports

    add_test(NAME coverage_setup
        COMMAND ${LCOV_EXECUTABLE}
            --zerocounters
            --base-directory ${PROJECT_BINARY_DIR}/src
            --directory ${PROJECT_BINARY_DIR}/src
    )
    set_tests_properties(coverage_setup PROPERTIES
        FIXTURES_SETUP COVERAGE
    )

    add_test(NAME coverage_generate
        COMMAND ${LCOV_EXECUTABLE}
            --capture
            --base-directory ${PROJECT_BINARY_DIR}/src
            --directory ${PROJECT_BINARY_DIR}/src
            --output-file ${CMAKE_BINARY_DIR}/Testing/coverage.lcov
            ${LCOV_EXCLUDES}
            --ignore-errors unused
    )
    set_tests_properties(coverage_generate PROPERTIES
        FIXTURES_CLEANUP COVERAGE
        FIXTURES_SETUP COVERAGE_REPORT
    )

    add_test(NAME coverage_report
        COMMAND ${GENHTML_EXECUTABLE}
            --demangle-cpp
            --output-directory ${CMAKE_BINARY_DIR}/Testing/coverage_report
            ${CMAKE_BINARY_DIR}/Testing/coverage.lcov
    )
    set_tests_properties(coverage_report PROPERTIES
        FIXTURES_REQUIRED COVERAGE_REPORT
        FIXTURES_CLEANUP COVERAGE
    )
endfunction()

function(_configure_lang_override_launcher LANG VAR PREFIX)
    message(STATUS "Configuring ${LANG} launcher")
    set(launcher ${CMAKE_BINARY_DIR}/${PREFIX}-launcher)
    set(OVERRIDE_VARIABLE LAUNCHER_OVERRIDE_${VAR})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/override-launcher.sh.in
        ${launcher}
        @ONLY
    )
    if (NOT CMAKE_${LANG}_COMPILER_LAUNCHER)
        set(CMAKE_${LANG}_COMPILER_LAUNCHER ${launcher}
            CACHE FILEPATH "${LANG} compiler launcher" FORCE
        )
        mark_as_advanced(CMAKE_${LANG}_COMPILER_LAUNCHER)
        list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
            CMAKE_${LANG}_COMPILER_LAUNCHER
        )
    endif()
    if (NOT CMAKE_${LANG}_LINKER_LAUNCHER)
        set(CMAKE_${LANG}_LINKER_LAUNCHER ${launcher}
            CACHE FILEPATH "${LANG} linker launcher" FORCE
        )
        mark_as_advanced(CMAKE_${LANG}_LINKER_LAUNCHER)
        list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
            CMAKE_${LANG}_LINKER_LAUNCHER
        )
    endif()
    return(PROPAGATE CMAKE_TRY_COMPILE_PLATFORM_VARIABLES)
endfunction()

function(configure_override_launcher)
    if (NOT AUTOCOMPAT_ENABLE_OVERRIDE_LAUNCHER)
        return()
    endif()
    if (NOT CMAKE_C_COMPILER_LAUNCHER OR
        NOT CMAKE_C_LINKER_LAUNCHER)
        _configure_lang_override_launcher(C CC cc)
    endif()
    if (NOT CMAKE_CXX_COMPILER_LAUNCHER OR
        NOT CMAKE_CXX_LINKER_LAUNCHER)
        _configure_lang_override_launcher(CXX CXX c++)
    endif()
    return(PROPAGATE CMAKE_TRY_COMPILE_PLATFORM_VARIABLES)
endfunction()
