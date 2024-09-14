#!/bin/bash
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

#
# Stop, kill or remove the containers that have been started by the
# test scripts.
#

scriptdir="$(cd "${0%/*}" && pwd)"
rootdir=$(realpath "$scriptdir/../..")

# shellcheck source=functions.sh
. "${rootdir}/tools/functions.sh"

usage() {
    echo "usage: $(basename "$0") [-hkr]"
    echo "  options:"
    echo "      -h|--help - show this help menu"
    echo "      -k|--kill - kill the containers instead of stopping them"
    echo "      -r|--remove - remove the container after it has been stopped"
    echo "      -u|--unique-id - unique id to filter container and network names"
}

main() {
    local stop_cmd remove unique_id
    stop_cmd="stop"
    if ! OPTS=$(getopt -o 'hkru:' --long help,kill,remove,unique-id: -n 'parse-options' -- "$@"); then
        err "Failed parsing options." >&2
        usage
        exit 1
    fi

    eval set -- "$OPTS"

    while true; do
        case "$1" in
            -h | --help)        usage; exit 0; shift ;;
            -k | --kill)        stop_cmd="kill"; shift;;
            -r | --remove)      remove=true; shift;;
            -u | --unique-id)   unique_id="$2"; shift 2 ;;
            -- ) shift; break ;;
            * ) err "unsupported argument $1"; usage; exit 1 ;;
        esac
    done

    local filter

    if [ -z "$unique_id" ] ; then
        # Filter all prplmesh labeled components
        echo "WARNING: Unique ID not specified, considering ALL prplmesh containers..."
        filter="label=prplmesh"
    else
        # Filter using the provided unique id
        filter="label=prplmesh-id=${unique_id}"
    fi

    local containers networks
    containers=$(docker ps -a -q --filter "${filter}" | xargs)
    networks=$(docker network ls -q --filter "${filter}" | xargs)

    dbg "Found containers: ${containers}"
    dbg "Found networks: ${networks}"

    # Stop the containers
    if [ -n "${containers}" ]; then
        echo "Stopping running containers for id: ${unique_id:-ALL}"
        # shellcheck disable=SC2086
        docker "$stop_cmd" ${containers} >/dev/null 2>&1 || true
    fi

    if [ "$remove" = true ] ; then
        # Remove the containers
        # shellcheck disable=SC2086
        [ -n "${containers}" ] && echo "Removing containers:" && docker rm ${containers}
        
        # Remove the networks
        # shellcheck disable=SC2086
        [ -n "${networks}" ] && echo "Removing networks:" && docker network rm ${networks}

        # Prune stopped containers and unused networks
        # Used as cleanup just in case previous removal operations failed
        echo "Removing stopped prplMesh containers and unused networks..."
        docker container prune -f --filter "label=prplmesh"
        docker network prune -f --filter "label=prplmesh"
    fi
}

main "$@"
