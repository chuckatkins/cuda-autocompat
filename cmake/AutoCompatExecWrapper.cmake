cmake_minimum_required(VERSION 3.19)

function(set_environment)
    foreach (ARG IN LISTS ARGV)
        if (NOT ARG MATCHES "^([^=]*)=(.*)$" )
            message(FATAL_ERROR "ENVIRONMENT must be a list of KEY=VALUE entries")
        endif()
        if (NOT CMAKE_MATCH_2)
            unset(ENV{${CMAKE_MATCH_1}})
        else()
            set(ENV{${CMAKE_MATCH_1}} "${CMAKE_MATCH_2}")
        endif()
    endforeach()
endfunction()

if (NOT EXE)
    message(FATAL_ERROR "EXE must be defined")
endif()

if (LIST_SEPARATOR)
    string(REPLACE "${LIST_SEPARATOR}" ";" EXE "${EXE}")
    if (ENVIRONMENT)
        string(REPLACE "${LIST_SEPARATOR}" ";" ENVIRONMENT "${ENVIRONMENT}")
    endif()
endif()

set(EP_OPTIONS)

if (INPUT_FILE)
    list(APPEND EP_OPTIONS INPUT_FILE ${INPUT_FILE})
endif()

if (OUTPUT_QUIET)
    list(APPEND EP_OPTIONS OUTPUT_QUIET)
elseif (OUTPUT_REGEX)
    list(APPEND EP_OPTIONS
        OUTPUT_VARIABLE RESULT_OUTPUT
        ECHO_OUTPUT_VARIABLE
    )
endif()
if (ERROR_QUIET)
    list(APPEND EP_OPTIONS ERROR_QUIET)
elseif (ERROR_REGEX)
    list(APPEND EP_OPTIONS
        ERROR_VARIABLE RESULT_ERROR
        ECHO_ERROR_VARIABLE
    )
endif()

set_environment(${ENVIRONMENT})

execute_process(
    ${EP_OPTIONS}
    RESULT_VARIABLE RESULT_RETURN
    COMMAND ${EXE}
)

if (NOT WILL_FAIL AND NOT RESULT_RETURN EQUAL 0)
    message(FATAL_ERROR "Command failed: ${RESULT_RETURN}")
elseif (WILL_FAIL AND RESULT_RETURN EQUAL 0)
    message(FATAL_ERROR "Command succeeded but was expected to fail")
endif()

if (OUTPUT_REGEX AND NOT (RESULT_OUTPUT MATCHES "${OUTPUT_REGEX}"))
    message(FATAL_ERROR "STDOUT does not match OUTPUT_REGEX: ${OUTPUT_REGEX}")
endif()

if (ERROR_REGEX AND NOT (RESULT_ERROR MATCHES "${ERROR_REGEX}"))
    message(FATAL_ERROR "STDERR does not match ERROR_REGEX: ${ERROR_REGEX}")
endif()
