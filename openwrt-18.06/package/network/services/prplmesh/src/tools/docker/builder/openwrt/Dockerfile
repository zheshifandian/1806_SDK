###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

####
## OpenWrt pre-requisites and "openwrt" user
####
FROM ubuntu:bionic-20210930 as openwrt-prerequisites

RUN apt-get update \
    && apt-get -y install --no-install-recommends \
    autoconf-archive build-essential libncurses5 libncurses5-dev python python3 \
    unzip git ca-certificates gawk wget file bash fakeroot \
    python-yaml python3-yaml rsync less vim gnupg software-properties-common \
    #   yq is available in a separate ppa (needs gnupg and software-properties-common):
    && if [ -n "$HTTP_PROXY" ]; then KEYSERVER_OPTIONS="--keyserver-options http-proxy=$HTTP_PROXY"; fi \
    && apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 $KEYSERVER_OPTIONS --recv-keys CC86BB64 \
    && add-apt-repository ppa:rmescandon/yq \
    && apt-get -y install --no-install-recommends yq \
    && rm -rf /var/lib/apt/lists/*
# Building tlvfs currently uses the docker
# container's python (not the one from OpenWrt)
# python-yaml is needed to build the tlvfs
# vim and less are there to debug builds at run time

RUN useradd -ms /bin/bash openwrt

####
## OpenWrt tree and scripts to build it.
####
FROM openwrt-prerequisites as openwrt-builder

USER openwrt

# The following args are mandatory, do not expect the build to
# succeed without specifying them on the command line.
#
# OpenWrt repository to use. Can also be a prplWrt repository:
ARG OPENWRT_REPOSITORY
ENV OPENWRT_REPOSITORY=$OPENWRT_REPOSITORY

# Target system to build for (used for prplwrt gen_config.py script):
# Example: TARGET_SYSTEM=mvebu
ARG TARGET_SYSTEM
ENV TARGET_SYSTEM=$TARGET_SYSTEM

WORKDIR /home/openwrt

# Checkout and build a fixed revision as a base. It's used to get a
# compiled toolchain for the target, and it's not meant to change
# often (it will be updated as required when OPENWRT_VERSION changes
# to a new release).
# Remove bin/ to make sure any artifact we save is regenerated with the version from the next steps.
# Remove some of the build directories because they are used with wildcard in the feed_intel
# Makefiles, and it might match two or the wrong directories
RUN printf '\033[1;35m%s Cloning prplWrt\n\033[0m' "$(date --iso-8601=seconds --universal)" \
    && git clone "$OPENWRT_REPOSITORY" openwrt \
    && printf '\033[1;35m%s Building base prplWrt tree\n\033[0m' "$(date --iso-8601=seconds --universal)" \
    && cd openwrt \
    && git checkout c741916862ebf2763aa4b8f0f5814ed4adef1510 \
    && ./scripts/gen_config.py "$TARGET_SYSTEM" \
    && make -j"$(nproc)" \
    && rm -rf bin \
    && rm -rf build_dir/target*/libnl-3* \
    && rm -rf build_dir/target*/openssl* \
    && rm -rf build_dir/target*/iwlwav*

# The following args are mandatory, do not expect the build to
# succeed without specifying them on the command line.
#
# Which OpenWrt version (commit hash) to use:
ARG OPENWRT_VERSION
ENV OPENWRT_VERSION=$OPENWRT_VERSION

RUN cd openwrt \
    && git fetch \
    && git checkout "$OPENWRT_VERSION"

# The variant to build (nl80211 or dwpal)
ARG PRPLMESH_VARIANT
ENV PRPLMESH_VARIANT=$PRPLMESH_VARIANT

# optional: whether to include MMX in the build or not.
ARG MMX_ENABLE
ENV MMX_ENABLE=$MMX_ENABLE

WORKDIR /home/openwrt/openwrt

COPY --chown=openwrt:openwrt scripts/build-openwrt.sh /home/openwrt/openwrt/scripts/build-openwrt.sh

####
## Prebuilt OpenWrt and prplMesh dependencies, but not the prplMesh ipk itself
####
FROM openwrt-builder as prplmesh-builder

RUN scripts/build-openwrt.sh
