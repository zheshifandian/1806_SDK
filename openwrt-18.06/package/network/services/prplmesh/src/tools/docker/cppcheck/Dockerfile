FROM debian:stable

# Use a specific snapshot to try to make the docker image a little bit more reproducible:
RUN printf '%s\n' 'deb     https://snapshot.debian.org/archive/debian/20210105T023154Z/ stable main' > /etc/apt/source.list.d

ARG PACKAGES_FOR_BUILD="\
    binutils \
    cmake \
    gcc \
    g++ \
    git \
    python3 \
    wget"

ARG PACKAGES_FOR_RUN="\
    bash \
    git"

ARG CPPCHECK_VERSION=2.4

RUN apt-get update && \
    apt-get install -y $PACKAGES_FOR_BUILD && \
    git clone https://github.com/danmar/cppcheck.git && \
    git -C cppcheck checkout "$CPPCHECK_VERSION" && \
    cmake -Hcppcheck -Bcppcheck -DUSE_MATCHCOMPILER=ON && \
    make -C cppcheck && \
    make -C cppcheck install && \
    rm -rf cppcheck && \
    apt-get purge -y --autoremove $PACKAGES_FOR_BUILD && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get install -y $PACKAGES_FOR_RUN && \
    rm -rf /var/lib/apt/lists/*
