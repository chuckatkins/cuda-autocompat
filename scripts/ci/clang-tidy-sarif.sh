#!/bin/bash

set -eo pipefail

if [ $# -ne 2 ]
then
    echo "Usage: $0 <source_dir> <build_dir>"
    exit 1
fi

source_dir="$1"
build_dir="$2"

config="${source_dir}/.clang-tidy"
comp_db="${build_dir}/compile_commands.json"

if [ ! -f "${config}" ]
then
    echo "error: .clang-tidy not found in ${source_dir}"
    exit 1
fi
if [ ! -f "${comp_db}" ]
then
    echo "error: compile_commands.json not found in ${build_dir}"
    exit 1
fi

for c in clang-tidy jq clang-tidy-sarif
do
    if ! command -v ${c} > /dev/null
    then
        echo "error: ${c} command not found"
        exit 1
    fi
done

source_files=( $(jq -r '.[].file' "${comp_db}") )
clang-tidy --config-file="${config}" -p="${comp_db}" "${source_files[@]}" | \
clang-tidy-sarif | \
sed 's|"name": "clang-tidy"|"name": "Clang Tidy"|' > clang-tidy.sarif
