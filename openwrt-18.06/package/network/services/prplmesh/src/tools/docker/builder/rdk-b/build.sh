#!/bin/sh -e
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

scriptdir="$(cd "${0%/*}"; pwd)"
rootdir="${scriptdir%/*/*/*/*}"

# shellcheck source=functions.sh
. "${rootdir}/tools/functions.sh"

usage() {
    echo "usage: $(basename "$0") -d <target_device> [-c <cache_dir>] [-t <tag>] [-hv]"
    echo "  options:"
    echo "      -d|--target-device the device to build for"
    echo "      -c|--cache - directory containing the yocto cache files (sstate, downloads)"
    echo "      -t|--tag - the tag to use for the builder image"
    echo "      -h|--help - show this help menu"
    echo "      -v|--verbose - increase the script's verbosity"
    echo " -d is always required."
    echo ""
    echo "The following environment variables will affect the build:"
    echo " - RDK_GIT_USER: the username with which to authenticate to rdk"
    echo " - RDK_GIT_TOKEN_FILE: file containing the password for authentication to rdk"
}

build_image() {
    info "Building image $image_tag"
    if [ -n "$RDK_GIT_TOKEN_FILE" ]; then
        RDK_GIT_TOKEN="$(base64 -d "$RDK_GIT_TOKEN_FILE")"
    fi
    if [ -z "$RDK_GIT_TOKEN" ]; then
        err "RDK_GIT_TOKEN not set, can't clone RDK-B!"
        exit 1
    fi

    # Docker doesn't allow symlinks pointing outside of the directory, so we need to copy it
    cp "${rootdir}/tools/get-git-hash.sh" "$scriptdir"
    docker build --tag "$image_tag" \
           --build-arg RDK_GIT_USER="$RDK_GIT_USER" \
           --build-arg RDK_GIT_TOKEN="$RDK_GIT_TOKEN" \
           --build-arg UID="$(id -u)" \
           --build-arg GID="$(id -g)" \
           "$scriptdir/"
}

build_prplmesh() {
    build_dir="$1"
    mkdir -p "$build_dir"
    mkdir -p "${CACHE_DIR}/downloads" "${CACHE_DIR}/sstate-cache"
    # Sometimes, the git cache gets corrupted: the directory is there, but it's not a git directory.
    # Run fsck on all git directories to overcome this.
    for dir in "${CACHE_DIR}/downloads/git2"/*/; do
        if ! (cd "$dir"; git rev-parse --is-inside-work-tree >/dev/null 2>&1); then
            echo "Remove broken git cache $dir"
            rm -rf "$dir"
        fi
    done
    # Make sure prplmesh is rebuilt
    rm -f "${CACHE_DIR}/"sstate-cache/*/sstate:prplmesh*
    docker run --rm --user "$(id -u):$(id -g)" \
           -v "$scriptdir/${TARGET_DEVICE}:/home/rdk/scripts:ro" \
           -v "${rootdir}:/home/rdk/prplMesh_source:ro" \
           -v "${CACHE_DIR}/downloads:/home/rdk/rdk-b/downloads" \
           -v "${CACHE_DIR}/sstate-cache:/home/rdk/rdk-b/sstate-cache" \
           -v "${build_dir}:/artifacts" \
           "$image_tag" \
           /home/rdk/scripts/build.sh
}

main() {

    if ! command -v uuidgen > /dev/null ; then
        err "You need uuidgen to use this script. Please install it and try again."
        exit 1
    fi

    if ! OPTS=$(getopt -o 'd:c:t:hv' --long target-device:,cache:,tag:,help,verbose -n 'parse-options' -- "$@"); then
        err "Failed parsing options." >&2
        usage
        exit 1
    fi

    eval set -- "$OPTS"

    while true; do
        case "$1" in
            -h | --help)               usage; exit 0; shift ;;
            -v | --verbose)            VERBOSE=true; shift ;;
            -d | --target-device)      TARGET_DEVICE="$2"; shift ; shift ;;
            -c | --cache)              CACHE_DIR="$2"; shift ; shift ;;
            -t | --tag)                TAG="$2"; shift ; shift ;;
            -- ) shift; break ;;
            * ) err "unsupported argument $1"; usage; exit 1 ;;
        esac
    done

    if [ -z "$TARGET_DEVICE" ]; then
        err "Target device is not set." >&2
        usage
        exit 1
    fi

    if [ ! -d "$scriptdir/$TARGET_DEVICE" ]; then
        err "Unknown device: $TARGET_DEVICE" >&2
        exit 1
    fi

    dbg "TARGET_DEVICE=$TARGET_DEVICE"
    dbg "TAG=$TAG"
    dbg "CACHE_DIR=$CACHE_DIR"

    if [ -n "$TAG" ] ; then
        image_tag="prplmesh-builder-rdk:$TAG"
    else
        image_tag="prplmesh-builder-rdk"
    fi

    build_image
    build_prplmesh "$rootdir/build/${TARGET_DEVICE}-rdk"

}

VERBOSE=false
CACHE_DIR="${rootdir}/rdk"

main "$@"
