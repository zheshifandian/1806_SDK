#! /bin/bash
###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2021 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

# Consistent with the Git Protocol Security Improvement on GitHub
# we can no longer use git://github. There are many places
# in the rdk-b recipes that use git: instead of https:
# As a temporary fix, we add the following line to the git config
git config --global url."https://github".insteadOf git://github

printf '\033[1;35m%s Configuring RDK-B\n\033[0m' "$(date --iso-8601=seconds --universal)"

export MACHINE=turris

# Not part of this repository
# shellcheck disable=SC1091
source meta-turris/setup-environment || {
    printf '\033[1;31mFailed to configure RDK-B\n\033[0m'
    exit 1
}

echo "BBLAYERS += \"\${RDKROOT}/meta-amx\"" >> conf/bblayers.conf
echo "BBLAYERS += \"\${RDKROOT}/meta-prplmesh\"" >> conf/bblayers.conf

# Copy in prplmesh. The build tries to write in the source directory, so we can't use mount
# directly.
rsync -a --exclude-from=/home/rdk/prplMesh_source/.gitignore \
    /home/rdk/prplMesh_source/ /home/rdk/prplMesh || {
    printf '\033[1;31mFailed to copy prplMesh source\n\033[0m'
    exit 1
}

# Update prplmesh feed to use already check-out source instead of download from git
cat >> conf/local.conf <<EOF

INHERIT += "externalsrc"
EXTERNALSRC_pn-prplmesh = "/home/rdk/prplMesh"
EOF

printf '\033[1;35m%s Building RDK-B\n\033[0m' "$(date --iso-8601=seconds --universal)"

bitbake rdk-generic-broadband-image
res=$?

printf '\033[1;35m%s Building done, result %d\n\033[0m' "$(date --iso-8601=seconds --universal)" "$res"

# Collect the artifacts
# For the log files, there's unfortunately no single directory that has them all. We also don't want
# to collect all "temp" directories, because the sources are intermingled in the work directory
# and they may contain "temp" directories as well. Therefore, try a few levels deep. Note that we
# may still accidentally hit a temp directory in the sources this way, but the chances are a bit
# smaller.
for tempdir in tmp/work/*/*/temp tmp/work/*/*/*/temp tmp/work/*/*/*/*/temp; do
    if [ -d "$tempdir" ]; then
        # shellcheck disable=SC2001
        package="$(echo "$tempdir" | sed 's%^tmp/work/\(.*\)/temp$%\1%')"
        mkdir -p /artifacts/logs/"$package"
        cp -r "$tempdir"/log* /artifacts/logs/"$package"
    fi
done

IMAGES_DIR="tmp/deploy/images/turris"
DESTINATION_DIR="/artifacts"
RDKB_ROOTFS=$(basename "$(readlink -f ${IMAGES_DIR}/rdk-generic-broadband-image-turris.tar.gz)")
RDKB_DTB=$(basename "$(find ${IMAGES_DIR} -name 'armada*omnia.dtb')")

cp "${IMAGES_DIR}/rdk-generic-broadband-image-turris.manifest" "${DESTINATION_DIR}"
cp "${IMAGES_DIR}/zImage" "${DESTINATION_DIR}"
cp "${IMAGES_DIR}/${RDKB_ROOTFS}" "${DESTINATION_DIR}"
cp "${IMAGES_DIR}/${RDKB_DTB}" "${DESTINATION_DIR}"
cp tmp/deploy/ipk/armv7ahf-neon/prplmesh* "${DESTINATION_DIR}"


exit $res
