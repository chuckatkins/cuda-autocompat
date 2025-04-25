#!/bin/bash
set -eo pipefail

echo "::group::Bash version"
"${BASH}" --version
echo "::endgroup::"

echo "::group::Environment"
env | sort
echo "::endgroup::"

echo "::group::CI environment setup"

function check_var() {
    local var_name=$1
    local -n var_value=${var_name}
    if [[ -z ${var_value} ]]
    then
        echo "error: ${var_name} is empty or unset"
        exit 1
    fi
    echo "${var_name}=${var_value}"
}

function check_and_export_var() {
    check_var $1
    export $1
}

function common_dir() {
    local rel="$(realpath --relative-to "$1" "$2")"
    local rel_only="$(expr "${rel}" : '\(\(\.\./\)\+\)')"
    realpath -Lms "$1/${rel_only}"
}

if [[ -z ${CI_SITE_NAME} && -n ${GITHUB_WORKFLOW} ]]
then
    CI_SITE_NAME="GitHub"
fi
check_and_export_var CI_SITE_NAME

if [[ -z ${CI_SOURCE_DIR} ]]
then
    CI_SOURCE_DIR="${CI_ROOT_DIR}/source"
elif [[ -n ${CI_SOURCE_DIR} ]]
then
    CI_SOURCE_DIR="$(realpath -Les "${CI_SOURCE_DIR}")"
fi
check_and_export_var CI_SOURCE_DIR

if [[ -z ${CI_BUILD_DIR} ]]
then
    CI_BUILD_DIR="${CI_ROOT_DIR}/build"
elif [[ -n ${CI_BUILD_DIR} ]]
then
    CI_BUILD_DIR="$(realpath -Lms "${CI_BUILD_DIR}")"
fi
check_and_export_var CI_BUILD_DIR

CI_ROOT_DIR="$(common_dir "${CI_SOURCE_DIR}" "${CI_BUILD_DIR}")"
check_and_export_var CI_ROOT_DIR

if [[ -z ${CI_BUILD_BASENAME} ]]
then
    if [[ ${GITHUB_EVENT_NAME} == pull_request ]]
    then
        echo "GITHUB_REF=${GITHUB_REF}"
        echo "GITHUB_HEAD_REF=${GITHUB_HEAD_REF}"
        GH_PR_NUMBER=$(expr "${GITHUB_REF}" : 'refs/pull/\([^/]*\)')
        CI_BUILD_BASENAME="pr${GH_PR_NUMBER}_${GITHUB_HEAD_REF}"
    elif [[ -n ${GITHUB_REF_NAME} ]]
    then
        echo "GITHUB_REF_NAME=${GITHUB_REF_NAME}"
        CI_BUILD_BASENAME="${GITHUB_REF_NAME}"
    elif [[ -d ${CI_SOURCE_DIR} ]]
    then
        if (cd "${CI_SOURCE_DIR}" && git rev-parse --is-inside-work-tree > /dev/null 2>&1)
        then
            CI_BUILD_BASENAME="$(cd "${CI_SOURCE_DIR}" && git symbolic-ref --short HEAD)"
        fi
    fi
fi
check_var CI_BUILD_BASENAME

if [[ -z ${CI_JOBNAME} ]]
then
    if [[ -n ${CI_LINTER} ]]
    then
        echo "CI_LINTER=${CI_LINTER}"
        CI_JOBNAME="${CI_LINTER}"
    elif [[ -n ${CI_COMPILER} ]]
    then
        echo "CI_COMPILER=${CI_COMPILER}"
        CI_JOBNAME="${CI_COMPILER}"
        if [[ -n ${CI_STDLIB} ]]
        then
            echo "CI_STDLIB=${CI_STDLIB}"
            CI_JOBNAME="${CI_JOBNAME}-${CI_STDLIB}"
        fi
        if [[ -n ${CI_RTLIB} ]]
        then
            echo "CI_RTLIB=${CI_RTLIB}"
            CI_JOBNAME="${CI_JOBNAME}-${CI_RTLIB}"
        fi
        if [[ -n ${CI_LINKER} ]]
        then
            echo "CI_LINKER=${CI_LINKER}"
            CI_JOBNAME="${CI_JOBNAME}-${CI_LINKER}"
        fi
    elif [[ -n ${GITHUB_JOB} ]]
    then
        echo "GITHUB_JOB=${GITHUB_JOB}"
        CI_JOBNAME="${GITHUB_JOB}"
    fi
fi
check_var CI_JOBNAME

CI_BUILD_NAME="${CI_BUILD_BASENAME}_${CI_JOBNAME}"
check_and_export_var CI_BUILD_NAME

if [[ -z ${CTEST} ]]
then
    CTEST="$(which ctest)"
fi
check_var CTEST

CTEST_SCRIPT="$(realpath -Les "$(dirname "${BASH_SOURCE[0]}")/../cmake/ci_common.cmake")"
check_var CTEST_SCRIPT

declare -A ctest_arg_map
ctest_arg_map["dashboard_do_checkout"]=FALSE
ctest_arg_map["dashboard_do_submit"]=TRUE
ctest_arg_map["dashboard_model"]=Experimental
ctest_arg_map["dashboard_group"]=Continuous

if [[ $# -eq 0 ]]
then
    ctest_arg_map["dashboard_do_full"]=TRUE
else
    ctest_arg_map["dashboard_do_full"]=FALSE
    dashboard_cache=""
    for arg in "$@"
    do
        if [[ ${arg} == *:*=* ]]
        then
            dashboard_cache+=$'\n'"${arg}"
        elif [[ ${arg} == -D*=* ]]
        then
            ctest_arg_map["$(expr match "${arg}" '-D\([^=]*\)')"]=${arg#-D*=}
        else
            ctest_arg_map["dashboard_do_${arg}"]=TRUE
        fi
    done
    if [[ -n ${dashboard_cache} ]]
    then
        ctest_arg_map["dashboard_cache"]="${dashboard_cache}"
    fi
fi
CTEST_ARGS=()
for key in "${!ctest_arg_map[@]}"
do
    CTEST_ARGS+=( -D${key}="${ctest_arg_map[${key}]}" )
done

echo "CTEST_ARGS="
for arg in "${CTEST_ARGS[@]}"
do
    echo "    ${arg}"
done
echo "::endgroup::"

echo "::group::CTest version"
"${CTEST}" --version
echo "::endgroup::"

export VERBOSE=1
echo "::group::Execute job step"
"${CTEST}" -VV -S "${CTEST_SCRIPT}" "${CTEST_ARGS[@]}"
RET=$?
echo "::endgroup::"

exit ${RET}
