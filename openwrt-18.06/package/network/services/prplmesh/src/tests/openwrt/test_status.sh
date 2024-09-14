#!/bin/sh
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

usage() {
    echo "usage: $(basename "$0") <target>"
}

if [ -z "$1" ] ; then
    usage
    exit 1
fi

TARGET="$1"

# The first start currently never succeeds, we need to restart it first.
echo "Attempting to start/restart prplMesh ..."
ssh "$TARGET" <<"EOF"
# Some devices still use the old path (outside of "scripts")
ln -s /opt/prplmesh/scripts/prplmesh_utils.sh /opt/prplmesh/prplmesh_utils.sh || true
/opt/prplmesh/prplmesh_utils.sh restart -d
TIMEOUT=30
for _ in $(seq 1 "$TIMEOUT") ; do
    if /opt/prplmesh/prplmesh_utils.sh status ; then
        exit 0
    fi
    sleep 1
done
exit 1
EOF
# Save exit status
TEST_STATUS=$?

echo "Stopping prplMesh"
ssh "$TARGET" /opt/prplmesh/prplmesh_utils.sh stop

exit $TEST_STATUS
