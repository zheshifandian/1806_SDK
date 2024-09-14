#!/bin/bash
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Apply UCI settings on a device.

set -o errexit
set -o nounset
set -o pipefail

usage() {
    echo "usage: $(basename "$0") [-h]"
    echo "  mandatory:"
    echo "      --target-name - the name of the target (will be used as the SSH destination)."
    echo "      --target-type - the type of the target. Example: turris-omnia"
    echo "  options:"
    echo "      -h|--help - show this help menu"
}

apply-settings(){
    # Check if the target has a corresponding uci script.
    # If it does, run it on the target.
    if [ -f "tests/certification/$TARGET_TYPE/uci.sh" ] ; then
        echo "Applying UCI configuration"
        scp "tests/certification/$TARGET_TYPE/uci.sh" "$TARGET_NAME":/tmp
        ssh "$TARGET_NAME" 'sh /tmp/uci.sh'
        echo "Done"
    else
        echo "Target $TARGET_NAME of type $TARGET_TYPE doesn't have any uci script to be applied."
    fi
}

main() {
    if ! OPTS=$(getopt -o 'h' --long help,target-name:,target-type: -n 'parse-options' -- "$@"); then
        echo "Failed parsing options." >&2
        usage
        exit 1
    fi

    eval set -- "$OPTS"

    while true; do
        case "$1" in
            -h|--help) usage; exit 0 ;;
            --target-name)
                TARGET_NAME="$2"
                shift 2
                ;;
            --target-type)
                TARGET_TYPE="$2"
                shift 2
                ;;
            -- ) shift; break ;;
            * ) echo "unsupported argument $1"; usage; exit 1 ;;
        esac
    done

    if ! ssh "$TARGET_NAME" /bin/true ; then
        echo "Error: $TARGET_NAME unreachable via ssh"
        echo "Cannot apply UCI settings, aborting."
        exit 1
    fi

    apply-settings "$@"
}

main "$@"
