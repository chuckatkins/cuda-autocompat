#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# "Universal" Dashboard Script
#
# This script contains basic dashboard driver code common to all
# clients and projects.  It is a combination of the universal.cmake script in
# the Kitware DashboardScriptsNG repo and cmake_common.cmake used by CMake
# dashboards.
#
# Create a project-specific common script with code of the following form,
# where the final line includes this script.
#
#   set(CTEST_PROJECT_NAME "OpenChemistry")
#   set(CTEST_DROP_SITE "cdash.openchemistry.org")
#
#   set(dashboard_git_url "git://source.openchemistry.org/openchemistry.git")
#   set(dashboard_source_name "openchemistry")
#
#   get_filename_component(dir ${CMAKE_CURRENT_LIST_FILE} PATH)
#   include(${dir}/universal.cmake)
#
# The following variables may be set before including this script
# to configure it:
#
#   dashboard_model       = Nightly | Experimental
#   dashboard_source_name = Name of source directory (source)
#   dashboard_binary_name = Name of binary directory (build)
#   dashboard_cache       = Initial CMakeCache.txt file content
#   dashboard_group       = The name of the CDash "Group" to submit to

#   dashboard_do_full      = True to enable all steps by default
#   dashboard_do_checkout  = True to enable source checkout via git
#   dashboard_do_update    = True to enable the Update step
#   dashboard_do_configure = True to enable the Configure step
#   dashboard_do_build     = True to enable the Build step
#   dashboard_do_test      = True to enable the Test step
#   dashboard_do_coverage  = True to enable coverage (ex: gcov)
#   dashboard_do_memcheck  = True to enable memcheck (ex: valgrind)
#   dashboard_do_done      = True to enable the Done submission
#   dashboard_do_submit    = Submit each step (ON)
#   dashboard_do_submit_only = Only submit step results, do not run them (OFF)

#   CTEST_GIT_COMMAND     = path to git command-line client
#   CTEST_BUILD_FLAGS     = build tool arguments (ex: -j2)
#   CTEST_DASHBOARD_ROOT  = Where to put source and build trees
#   CTEST_TEST_CTEST      = Whether to run long CTestTest* tests
#   CTEST_TEST_TIMEOUT    = Per-test timeout length
#   CTEST_TEST_ARGS       = ctest_test args (ex: PARALLEL_LEVEL 4)
#   CMAKE_MAKE_PROGRAM    = Path to "make" tool to use
#
# Options to configure Git:
#   dashboard_git_url      = Custom git clone url
#   dashboard_git_branch   = Custom remote branch to track
#   dashboard_git_crlf     = Value of core.autocrlf for repository
#
# For Makefile generators the script may be executed from an
# environment already configured to use the desired compilers.
# Alternatively the environment may be set at the top of the script:
#
#   set(ENV{CC}  /path/to/cc)   # C compiler
#   set(ENV{CXX} /path/to/cxx)  # C++ compiler
#   set(ENV{FC}  /path/to/fc)   # Fortran compiler (optional)
#   set(ENV{LD_LIBRARY_PATH} /path/to/vendor/lib) # (if necessary)

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(NOT DEFINED dashboard_do_full)
  set(dashboard_do_full TRUE)
endif()

# Initialize all build steps to "ON"
if(NOT DEFINED dashboard_do_update)
  set(dashboard_do_update ${dashboard_do_full})
endif()

if(NOT DEFINED dashboard_do_checkout)
  set(dashboard_do_checkout ${dashboard_do_full})
endif()

if(NOT DEFINED dashboard_do_configure)
  set(dashboard_do_configure ${dashboard_do_full})
endif()

if(NOT DEFINED dashboard_do_build)
  set(dashboard_do_build ${dashboard_do_full})
endif()

if(NOT DEFINED dashboard_do_test)
  set(dashboard_do_test ${dashboard_do_full})
endif()

# Default code coverage and memtesting to off
if(NOT DEFINED dashboard_do_coverage)
  set(dashboard_do_coverage FALSE)
endif()

if(NOT DEFINED dashboard_do_memcheck)
  set(dashboard_do_memcheck FALSE)
endif()

if(NOT DEFINED dashboard_do_done)
  set(dashboard_do_done ${dashboard_do_full})
endif()

if(NOT DEFINED dashboard_fresh)
  if(dashboard_do_full OR dashboard_do_update)
    set(dashboard_fresh TRUE)
  else()
    set(dashboard_fresh FALSE)
  endif()
endif()

if(NOT DEFINED dashboard_do_submit_only)
  set(dashboard_do_submit_only FALSE)
endif()

if(NOT DEFINED dashboard_do_submit)
  set(dashboard_do_submit TRUE)
endif()

if(NOT DEFINED CTEST_PROJECT_NAME)
  message(FATAL_ERROR "project-specific script including 'universal.cmake' should set CTEST_PROJECT_NAME")
endif()

if(NOT DEFINED dashboard_user_home)
  set(dashboard_user_home "$ENV{HOME}")
endif()

# Select the top dashboard directory.
if(NOT DEFINED CTEST_DASHBOARD_ROOT)
  get_filename_component(CTEST_DASHBOARD_ROOT "${CTEST_SCRIPT_DIRECTORY}/../../.." ABSOLUTE)
endif()

# Select the model (Nightly, Experimental, Continuous).
if(NOT DEFINED dashboard_model)
  set(dashboard_model Nightly)
endif()
if(NOT "${dashboard_model}" MATCHES "^(Nightly|Experimental)$")
    message(FATAL_ERROR "dashboard_model must be Nightly or Experimental")
endif()

# Default to a Debug build.
if(NOT DEFINED CTEST_BUILD_CONFIGURATION)
  set(CTEST_BUILD_CONFIGURATION Debug)
endif()

if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
  set(CTEST_CONFIGURATION_TYPE ${CTEST_BUILD_CONFIGURATION})
endif()

# Choose CTest reporting mode.
if(NOT DEFINED CTEST_USE_LAUNCHERS OR
   NOT "${CTEST_CMAKE_GENERATOR}" MATCHES "Make|Ninja")
  # Launchers work only with Makefile and Ninja generators.
  set(CTEST_USE_LAUNCHERS 0)
endif()

# Configure testing.
if(NOT DEFINED CTEST_TEST_CTEST)
  set(CTEST_TEST_CTEST 1)
endif()
if(NOT CTEST_TEST_TIMEOUT)
  set(CTEST_TEST_TIMEOUT 1500)
endif()

# Select Git source to use.
if(dashboard_do_checkout)
  if(NOT DEFINED dashboard_git_url)
    message(FATAL_ERROR "project-specific script including 'universal.cmake' should set dashboard_git_url")
  endif()
  if(NOT DEFINED dashboard_git_branch)
    set(dashboard_git_branch main)
  endif()
  if(NOT DEFINED dashboard_git_crlf)
    if(UNIX)
      set(dashboard_git_crlf false)
    else()
      set(dashboard_git_crlf true)
    endif()
  endif()

  # Look for a GIT command-line client.
  if(NOT DEFINED CTEST_GIT_COMMAND)
    find_program(CTEST_GIT_COMMAND
      NAMES git git.cmd
      PATH_SUFFIXES Git/cmd Git/bin
      )
  endif()
  if(NOT CTEST_GIT_COMMAND)
    message(FATAL_ERROR "CTEST_GIT_COMMAND not available!")
  endif()
endif()

# Select a source directory name.
if (NOT DEFINED dashboard_source_name)
  set(dashboard_source_name source)
endif()
if(NOT DEFINED CTEST_SOURCE_DIRECTORY)
  set(CTEST_SOURCE_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${dashboard_source_name})
endif()

# Select a build directory name.
if (NOT DEFINED dashboard_binary_name)
  set(dashboard_binary_name build)
endif()
if(NOT DEFINED CTEST_BINARY_DIRECTORY)
  set(CTEST_BINARY_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${dashboard_binary_name})
endif()

macro(dashboard_git)
  execute_process(
    COMMAND ${CTEST_GIT_COMMAND} ${ARGN}
    WORKING_DIRECTORY "${CTEST_SOURCE_DIRECTORY}"
    OUTPUT_VARIABLE dashboard_git_output
    ERROR_VARIABLE dashboard_git_output
    RESULT_VARIABLE dashboard_git_failed
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    )
endmacro()

if(dashboard_do_checkout)
  # Delete source tree if it is incompatible with current VCS.
  if(EXISTS ${CTEST_SOURCE_DIRECTORY})
    if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}/.git")
      set(vcs_refresh "because it is not managed by git.")
    else()
      execute_process(
        COMMAND ${CTEST_GIT_COMMAND} reset --hard
        WORKING_DIRECTORY "${CTEST_SOURCE_DIRECTORY}"
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        RESULT_VARIABLE failed
        )
      if(failed)
        set(vcs_refresh "because its .git may be corrupted.")
      endif()
    endif()
    if(vcs_refresh AND "${CTEST_SOURCE_DIRECTORY}" MATCHES "/CMake[^/]*")
      message("Deleting source tree\n")
      message("  ${CTEST_SOURCE_DIRECTORY}\n${vcs_refresh}")
      file(REMOVE_RECURSE "${CTEST_SOURCE_DIRECTORY}")
    endif()
  endif()

  # Support initial checkout if necessary.
  if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}"
      AND NOT DEFINED CTEST_CHECKOUT_COMMAND)
  # Generate an initial checkout script.
  get_filename_component(_name "${CTEST_SOURCE_DIRECTORY}" NAME)
  set(ctest_checkout_script ${CTEST_DASHBOARD_ROOT}/${_name}-init.cmake)
  file(WRITE ${ctest_checkout_script} "# git repo init script for ${_name}
execute_process(
  COMMAND \"${CTEST_GIT_COMMAND}\" clone -n -- \"${dashboard_git_url}\"
          \"${CTEST_SOURCE_DIRECTORY}\"
  )
if(EXISTS \"${CTEST_SOURCE_DIRECTORY}/.git\")
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" config core.autocrlf ${dashboard_git_crlf}
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" fetch
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" checkout ${dashboard_git_branch}
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
endif()"
  )
  set(CTEST_CHECKOUT_COMMAND "\"${CMAKE_COMMAND}\" -P \"${ctest_checkout_script}\"")
  elseif(EXISTS "${CTEST_SOURCE_DIRECTORY}/.git")
    # Upstream URL.
    dashboard_git(config --get remote.origin.url)
    if(NOT dashboard_git_output STREQUAL "${dashboard_git_url}")
      dashboard_git(config remote.origin.url "${dashboard_git_url}")
    endif()

    # Local checkout.
    dashboard_git(symbolic-ref HEAD)
    if(NOT dashboard_git_output STREQUAL "${dashboard_git_branch}")
      dashboard_git(checkout ${dashboard_git_branch})
      if(dashboard_git_failed)
        message(FATAL_ERROR "Failed to checkout branch ${dashboard_git_branch}:\n${dashboard_git_output}")
      endif()
    endif()
  endif()
endif()

#-----------------------------------------------------------------------------

# Check for required variables.
foreach(req
    CTEST_CMAKE_GENERATOR
    CTEST_SITE
    CTEST_BUILD_NAME
    )
  if(NOT DEFINED ${req})
    message(FATAL_ERROR "The containing script must set ${req}")
  endif()
endforeach(req)

# Print summary information.
set(vars "")
foreach(v
    CTEST_SITE
    CTEST_BUILD_NAME
    CTEST_SOURCE_DIRECTORY
    CTEST_BINARY_DIRECTORY
    CTEST_CMAKE_GENERATOR
    CTEST_BUILD_CONFIGURATION
    CTEST_GIT_COMMAND
    CTEST_CHECKOUT_COMMAND
    CTEST_CONFIGURE_COMMAND
    CTEST_SCRIPT_DIRECTORY
    CTEST_USE_LAUNCHERS
    )
  set(vars "${vars}  ${v}=[${${v}}]\n")
endforeach(v)
message("Dashboard script configuration:\n${vars}\n")

# Avoid non-ascii characters in tool output.
set(ENV{LC_ALL} C)

# Helper macro to write the initial cache.
macro(write_cache)
  set(cache_build_type "")
  set(cache_make_program "")
  if(CTEST_CMAKE_GENERATOR MATCHES "Make|Ninja")
    set(cache_build_type CMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION})
    if(CMAKE_MAKE_PROGRAM)
      set(cache_make_program CMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM})
    endif()
  endif()
  file(WRITE ${CTEST_BINARY_DIRECTORY}/CMakeCache.txt "
SITE:STRING=${CTEST_SITE}
BUILDNAME:STRING=${CTEST_BUILD_NAME}
CTEST_TEST_CTEST:BOOL=${CTEST_TEST_CTEST}
CTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
DART_TESTING_TIMEOUT:STRING=${CTEST_TEST_TIMEOUT}
GIT_EXECUTABLE:FILEPATH=${CTEST_GIT_COMMAND}
${cache_build_type}
${cache_make_program}
${dashboard_cache}
")
endmacro(write_cache)

if(COMMAND dashboard_hook_init)
  dashboard_hook_init()
endif()

if(dashboard_fresh)
  if(EXISTS "${CTEST_BINARY_DIRECTORY}")
    message("Clearing build tree...")
    ctest_empty_binary_directory("${CTEST_BINARY_DIRECTORY}")
  else()
    file(MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}")
  endif()
  message("Starting fresh build...")
  write_cache()
endif()

# Start a new submission.
if(dashboard_group)
    set(dashboard_group_arg GROUP "${dashboard_group}")
endif()
message("Calling ctest_start")
if(dashboard_fresh)
  if(COMMAND dashboard_hook_start)
    dashboard_hook_start()
  endif()
  ctest_start(${dashboard_model} ${dashboard_group_arg})
  if(dashboard_do_submit)
    ctest_submit(PARTS Start)
  endif()
  if(COMMAND dashboard_hook_started)
    dashboard_hook_started()
  endif()
else()
  ctest_start(${dashboard_model} ${dashboard_group_arg} APPEND)
endif()

# Look for updates.
if(dashboard_do_update)
  message("Dashboard step: update")
  if(NOT dashboard_do_submit_only)
    if(COMMAND dashboard_hook_update)
      dashboard_hook_update()
    endif()
    message("Calling ctest_update...")
    ctest_update(RETURN_VALUE count)
    set(CTEST_CHECKOUT_COMMAND) # checkout on first iteration only
    message("Found ${count} changed files")
  endif()

  if(dashboard_do_submit)
    if(CTEST_SUBMIT_NOTES)
      message("Submitting dashboard scripts as Notes")
      # Send the main script as a note while submitting the Update part
      set(CTEST_NOTES_FILES
        ${CTEST_UPDATE_NOTES_FILES}
        "${CMAKE_CURRENT_LIST_FILE}")
      ctest_submit(PARTS Update Notes)
      unset(CTEST_NOTES_FILES)
    else()
      message("Skipping notes submission for Update step")
      ctest_submit(PARTS Update)
    endif()
  endif()
endif()

if(dashboard_do_configure)
  message("Dashboard step: configure")
  if(NOT dashboard_do_submit_only)
    if(COMMAND dashboard_hook_configure)
      dashboard_hook_configure()
    endif()
    message("Calling ctest_configure")
    ctest_configure(${dashboard_configure_args})
  endif()
  if(dashboard_do_submit)
    if(CTEST_SUBMIT_NOTES)
      message("Submitting CMakeCache.txt as Notes")
      set(CTEST_NOTES_FILES "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
      ctest_submit(PARTS Configure Notes)
      unset(CTEST_NOTES_FILES)
    else()
      message("Skipping notes submission for Configure step")
      ctest_submit(PARTS Configure)
    endif()
  endif()
endif()

ctest_read_custom_files(${CTEST_BINARY_DIRECTORY})

if(dashboard_do_build)
  message("Dashboard step: build")
  if(NOT dashboard_do_submit_only)
    if(COMMAND dashboard_hook_build)
      dashboard_hook_build()
    endif()
    message("Calling ctest_build with CTEST_BUILD_FLAGS: " ${CTEST_BUILD_FLAGS})
    ctest_build(
      NUMBER_WARNINGS ctest_build_num_warnings
      )
  endif()
  if(dashboard_do_submit)
    ctest_submit(PARTS Build)
  endif()
endif()

if(dashboard_do_test)
  message("Dashboard step: test")
  if(NOT dashboard_do_submit_only)
    if(COMMAND dashboard_hook_test)
      dashboard_hook_test()
    endif()
    message("Calling ctest_test with CTEST_TEST_ARGS: " ${CTEST_TEST_ARGS})
    ctest_test(${CTEST_TEST_ARGS} RETURN_VALUE TEST_RESULTS)
    if(${TEST_RESULTS} EQUAL 0)
      message("ctest test results return value: ${TEST_RESULTS}")
    else()
      message(SEND_ERROR "Some tests failed")
    endif()
  endif()
  if(dashboard_do_submit)
    ctest_submit(PARTS Test)
  endif()
endif()

if(dashboard_do_coverage)
  message("Dashboard step: coverage")
  if(NOT dashboard_do_submit_only)
    if(COMMAND dashboard_hook_coverage)
      dashboard_hook_coverage()
    endif()
    message("Calling ctest_coverage")
    ctest_coverage()
  endif()
  if(dashboard_do_submit)
    ctest_submit(PARTS Coverage)
  endif()
endif()

if(dashboard_do_memcheck)
  message("Dashboard step: memcheck")
  if(NOT dashboard_do_submit_only)
    if(COMMAND dashboard_hook_memcheck)
      dashboard_hook_memcheck()
    endif()
    message("Calling ctest_memcheck with CTEST_MEMCHECK_ARGS: " ${CTEST_MEMCHECK_ARGS})
    ctest_memcheck(${CTEST_MEMCHECK_ARGS} RETURN_VALUE MEMCHECK_RETURN DEFECT_COUNT MEMCHECK_DEFECTS)
    if(NOT (MEMCHECK_DEFECTS EQUAL 0))
      message(SEND_ERROR "ctest memcheck defects found: ${MEMCHECK_DEFECTS}")
    endif()
  endif()
  if(dashboard_do_submit)
    ctest_submit(PARTS MemCheck)
  endif()
endif()

if(dashboard_do_done)
  message("Dashboard step: done")
  if(dashboard_do_submit)
    ctest_submit(PARTS Done)
  endif()
endif()

if(COMMAND dashboard_hook_end)
  dashboard_hook_end()
endif()
