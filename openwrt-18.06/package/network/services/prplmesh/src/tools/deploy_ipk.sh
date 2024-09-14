#!/bin/sh -e
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

usage() {
    echo "usage: $(basename "$0") <user>@<target> <ipk> [-h]"
    echo "  options:"
    echo "      -h|--help - show this help menu"
    echo "      --certification-mode - enable certification-mode on the target"
    echo "      --proxy - use ssh proxy <user>@<proxy>"
}

deploy() {
    if ! eval ssh "$SSH_OPTIONS" "$1" /bin/true ; then
        echo "Error: $1 unreachable via ssh"
        exit 1
    fi

    TARGET="$1"
    IPK="$2"
    IPK_FILENAME="$(basename "$2")"
    DEST_FOLDER=/tmp/prplmesh_ipks

    echo "Removing previous ipks"
    eval ssh "$SSH_OPTIONS" "$TARGET" \""rm -rf \"$DEST_FOLDER\" ; mkdir -p \"$DEST_FOLDER\"\""
    echo "Copying $IPK to $TARGET:$DEST_FOLDER/$IPK_FILENAME"
    eval scp "$SSH_OPTIONS" "$IPK" "$TARGET:$DEST_FOLDER/$IPK_FILENAME"

    # get board type
    BOARD_TYPE=$(eval ssh "$SSH_OPTIONS" "$TARGET" \""grep '^ID' -- /etc/os-release | cut -d '=' -f 2"\")
    echo "BOARD_TYPE=$BOARD_TYPE"

    eval ssh "$SSH_OPTIONS" "$TARGET" <<EOF

# we don't want opkg to stay locked with a previous failed invocation.
# when using this, make sure no one is using opkg in the meantime!
pgrep opkg | xargs kill -s INT
# remove any previously installed prplmesh. Use force-depends in case
# there are packages depending on prplmesh which will cause opkg remove
# to fail without this:
opkg remove --force-depends prplmesh prplmesh-dwpal prplmesh-nl80211
# currently opkg remove does not remove everything from /opt/prplmesh:
rm -rf /opt/prplmesh
# rdkb platforms require --force-dependencies
if [ "$BOARD_TYPE" = "rdk" ]; then 
    opkg install -V2 --force-depends "$DEST_FOLDER/$IPK_FILENAME"; 
else
    opkg install -V2  "$DEST_FOLDER/$IPK_FILENAME"; 
fi
EOF

    if [ "$CERTIFICATION_MODE" = true ] && [ "$BOARD_TYPE" != "rdk" ]; then
        echo "Certification mode will be enabled on the target"
        eval ssh "$SSH_OPTIONS" "$TARGET" \""uci set prplmesh.config.certification_mode=1 && uci commit"\"
        echo "Certification mode enabled on the target."
    fi

    if [ "$BOARD_TYPE" != "rdk" ]; then
        echo "Restarting networking"
        eval ssh "$SSH_OPTIONS" "$TARGET" \""/etc/init.d/network restart"\"
        echo "Done"
    fi
}

main() {
    if ! OPTS=$(getopt -o 'h' --long help,certification-mode,proxy: -n 'parse-options' -- "$@"); then
        echo "Failed parsing options." >&2
        usage
        exit 1
    fi

    eval set -- "$OPTS"

    while true; do
        case "$1" in
            -h|--help) usage; exit 0 ;;
            --certification-mode)
                CERTIFICATION_MODE=true
                shift
                ;;
            --proxy)
                SSH_OPTIONS="$SSH_OPTIONS \"-oProxyJump \"$2\"\""
                shift 2
                ;;
            -- ) shift; break ;;
            * ) echo "unsupported argument $1"; usage; exit 1 ;;
        esac
    done

    deploy "$@"
}

SSH_OPTIONS="\"-oBatchMode yes\" \"-oStrictHostKeyChecking no\""

main "$@"
