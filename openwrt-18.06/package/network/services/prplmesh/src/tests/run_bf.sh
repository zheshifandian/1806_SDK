#!/bin/bash
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

scriptdir=$(dirname "$(readlink -f "${0}")")
bf_plugins_dir=${scriptdir}/boardfarm_plugins
resultdir=${scriptdir}/../logs/

# parse command line arguments
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --dut)
            shift
            [[ "$#" -eq 0 ]] && echo "no device specified" && exit 1
            DUT="$1"
            shift
            ;;
        --test-suite)
            shift
            [[ "$#" -eq 0 ]] && echo "no test suite specified" && exit 1
            if [[ "$1" =~ ^TEST_LIST:(.*)$ ]]; then
                # Boardfarm implied interactive mode if tests are supplied via command line.
                # So we have to create a temporary file with the tests wrapped in a test suite.

                TESTS="${BASH_REMATCH[1]}"
                echo "Preparing environment to run following tests: ${TESTS}"

                temp_bf_plugins_dir="$(mktemp -d -t boardfarm_plugins-XXXX)"
                trap 'rm -rf "$temp_bf_plugins_dir"' EXIT

                echo "Copying ${bf_plugins_dir} to ${temp_bf_plugins_dir}"
                cp -R "${bf_plugins_dir}/." "${temp_bf_plugins_dir}"

                bf_plugins_dir="${temp_bf_plugins_dir}"

                test_suites_file="${bf_plugins_dir}/boardfarm_prplmesh/testsuites.cfg"
                echo "Generating test suite file in ${test_suites_file}"

                echo "[testsuite]" > "$test_suites_file"
                echo "${TESTS}" | tr ',' '\n' >> "$test_suites_file"

                TEST_SUITE="testsuite"
            else
                TEST_SUITE="$1"
            fi
            shift
            ;;
        *)
            echo "unsupported arg: $1"
            ;;
    esac
done

if [ -n "${PYTHONPATH}" ]; then
   PYTHONPATH="${bf_plugins_dir}:${scriptdir}:${PYTHONPATH}"
else
   PYTHONPATH="${bf_plugins_dir}:${scriptdir}"
fi
echo "$PYTHONPATH"
export PYTHONPATH
export BFT_DEBUG=y

bft -c "${bf_plugins_dir}"/boardfarm_prplmesh/prplmesh_config.json \
        -n "$DUT" -x "$TEST_SUITE" -o "${resultdir}" || exit 255

mapfile -t failed <<< "$(jq -c '.test_results[] | select(.grade == "FAIL") | .name' "${resultdir}"/test_results.json)"
mapfile -t passed <<< "$(jq -c '.test_results[] | select(.grade == "OK") | .name' "${resultdir}"/test_results.json)"
mapfile -t skipped <<< "$(jq -c '.test_results[] | select(.grade == null) | .name' "${resultdir}"/test_results.json)"

if [[ -n "${passed[*]/$'\n'/}" ]]; then
    if [[ -n "${failed[*]/$'\n'/}" ]]; then
        printf '\n\033[2;32m%s\033[0m\n' "${#passed[@]} tests passed!"
        for test in ${passed[*]};do
            printf '%s\n' "$test"
        done
    else
        printf '\n\033[2;32mAll tests passed!\033[0m\n\n'
    fi
fi

exit_code=0
if [[ -n "${skipped[*]/$'\n'/}" ]]; then
    printf '\n\033[1;36m%s\033[0m\n' "${#skipped[@]} tests skipped!"
    for test in ${skipped[*]};do
        printf '%s\n' "$test"
    done

    exit_code=1
fi

if [[ -n "${failed[*]/$'\n'/}" ]]; then
    printf '\n\033[1;31m%s\033[0m\n' "${#failed[@]} tests failed!"
    for test in ${failed[*]};do
        printf '%s\n' "$test"
    done

    exit_code=1
fi

exit $exit_code
