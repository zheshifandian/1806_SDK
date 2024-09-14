###############################################################
# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.
###############################################################

FROM ubuntu:bionic-20210930

ARG PACKAGES_FOR_BUILD="\
    binutils \
    cmake \
    gcc \
    git \
    libjson-c-dev \
    libncurses-dev \
    libnl-3-dev \
    libnl-genl-3-dev \
    libnl-route-3-dev \
    libreadline-dev \
    libssl-dev \
    ninja-build \
    pkg-config \
    python3 \
    python3-yaml \
    "

ARG PACKAGES_FOR_AMBIORIX="\
    bison \
    curl \
    flex \
    libevent-dev \
    libyajl-dev \
    libxslt1-dev \
    lua5.1 \
    liblua5.1-0-dev \
    build-essential \
    libxml2-dev \
    "

ARG PACKAGES_FOR_TEST="\
    clang-format \
    iperf3 \
    gcovr \
    python3-pytest \
    "

ARG PACKAGES_FOR_RUN="\
    bridge-utils \
    ebtables \
    iproute2 \
    net-tools \
    psmisc \
    uuid-runtime \
    "

ARG PACKAGES_FOR_DEBUG="\
    gdb \
    iputils-ping \
    netcat \
    valgrind \
    vim \
    "

ARG PACKAGES="\
    $PACKAGES_FOR_BUILD \
    $PACKAGES_FOR_AMBIORIX \
    $PACKAGES_FOR_TEST \
    $PACKAGES_FOR_RUN \
    $PACKAGES_FOR_DEBUG\
    "

RUN apt-get update && apt-get install -y $PACKAGES && \
    # We need liburiparser-dev > v. 9.0 which isn't avalaible in default
    # 18.04 Ubuntu repos. So add Ubuntu 19.10 repo which has v. 0.9.3-2
    echo "deb http://old-releases.ubuntu.com/ubuntu eoan main universe" | tee -a  /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y \
    liburiparser-dev && \
    rm -rf /var/lib/apt/lists/*

RUN curl https://storage.googleapis.com/git-repo-downloads/repo > /usr/bin/repo && \
    chmod +x /usr/bin/repo

WORKDIR ambiorix

# Fetch, checkout on fixed commit which works and install libubox
RUN \
    git clone git://git.openwrt.org/project/libubox.git && \
    cd libubox && git checkout 9e52171d70def760a6949676800d0b73f85ee22d && \
    cmake -H"$(pwd)"/. -B"$(pwd)"/build/. -DCMAKE_INSTALL_PREFIX=/usr && \
    make install -C build

# Fetch, checkout on fixed commit which works, patch and install Ubus daemon
ADD ubus/0001-ubusd-convert-tx_queue-to-linked-list.patch .
RUN \
    git clone git://git.openwrt.org/project/ubus.git && \
    cd ubus && git checkout 13a4438b4ebdf85d301999e0a615640ac4c9b0a8 && \
    patch -p1 -i ../0001-ubusd-convert-tx_queue-to-linked-list.patch && \
    cmake -H"$(pwd)"/. -B"$(pwd)"/build/. -DCMAKE_INSTALL_PREFIX=/usr && \
    make install -C build

# Fetch and intall Bus Agnostic API libs, applications.
# As they have some internal dependencies - we should build & install
# them in specific order.
RUN \
    repo init -u https://gitlab.com/prpl-foundation/components/ambiorix/ambiorix.git \
        -b refs/tags/v1.15.2 < /dev/null && \
    repo sync && \
    make install -C libraries/libamxc && \
    make install -C libraries/libamxp && \
    make install -C libraries/libamxd && \
    make install -C libraries/libamxj && \
    make install -C libraries/libamxo && \
    make install -C libraries/libamxb && \
    make install -C applications/amxb-inspect && \
    make install -C applications/amxo-cg && \
    make install -C applications/amxo-xml-to && \
    make -C bus_adaptors/amxb_ubus && \
    make install -C bus_adaptors/amxb_ubus

ADD start-prplmesh.sh /root/
