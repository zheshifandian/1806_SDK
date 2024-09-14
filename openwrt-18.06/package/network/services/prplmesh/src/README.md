# prplMesh
[![Build Status](https://gitlab.com/prpl-foundation/prplmesh/prplMesh/badges/master/pipeline.svg)](https://gitlab.com/prpl-foundation/prplmesh/prplMesh/pipelines)

prplMesh is an open-source, carrier-grade and certifiable implementation of the WiFi Alliance’s Multi-AP specification.

The result will be source-code covering both the agent and the controller part of the Multi-AP specification.
However, it is scoped as a reference implementation and will leave ample room for differentiation, for example for proprietary IP algorithms making intelligent decisions for the whole Multi-AP network.

In short, the project’s aim is to create a baseline for OEMs and developers to easily integrate Multi-AP into various products and platforms.
Initial targets include prplWrt and RDK-B with support for WiFi chipsets from almost any SoC vendor to be used in residential gateways, WiFi extenders from both retail brands and internet service providers.

This project is part of a wider collaboration between Prpl Foundation and Broadband Forum, and is based on a proven full mesh solution contributed by Intel Corp (Controller and Agent running on actual HW).

Architecture documentation can be found in the [documentation](documentation/) folder.

The latest build artifacts are [always accessible](https://ftp.essensium.com/owncloud/index.php/s/xidrhY3JKEYS9dK?path=%2Fartifacts%2Flatest%2Fbuild).

## Fetch Sources

If you haven't done so already, set up your git configuration:

```bash
git config --global user.email your@email.address
git config --global user.name "Your Name"
```

If you already have a gitlab account:
```bash
git clone ssh://git@gitlab.com/prpl-foundation/prplmesh/prplMesh.git
```
Otherwise
```bash
git clone https://gitlab.com/prpl-foundation/prplmesh/prplMesh.git
```

## Build in Docker

See corresponding [README](tools/docker/README.md)

## Native Build

As an alternative to the manual steps outlined below, [tools/maptools.py](tools/README.md) can be used to build and install prplMesh with a single command.

### Requirements

An up-to-date list of packages you need to build prplMesh on Ubuntu (18.04) is available in the [Dockerfile](tools/docker/builder/ubuntu/bionic/Dockerfile)
This is used in our automated builds, so is guaranteed to be up to date.
As of the time of writing, it includes the following packages:

* binutils 
* cmake 
* gcc 
* git 
* libjson-c-dev 
* libncurses-dev 
* libnl-3-dev 
* libnl-genl-3-dev 
* libnl-route-3-dev 
* libreadline-dev 
* libssl-dev 
* ninja-build 
* pkg-config 
* python 
* python-yaml 
* python3 
* python3-yaml 
* bison 
* curl 
* flex 
* libevent-dev 
* libyajl-dev 
* lua5.1 
* liblua5.1-0-dev 
* build-essential 
* clang-format 
* gcovr 
* bridge-utils 
* ebtables 
* iproute2 
* net-tools 
* psmisc 
* uuid-runtime

### Run Build

Use standard CMake to build prplMesh, with a configure-build-install cycle.

To build prplMesh natively in debug mode (for being able to debug with gdb), with all features and tests, and installed in a local directory, run

```bash
cmake -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../build/install -H. -B../build -G Ninja
ninja -C ../build install
```

If you prefer, `make` can be used instead of `ninja` by removing the `-G Ninja` part in 1st command and by replacing `ninja` with `make` in 2nd one.

### Install

For system-level install, the standard DESTDIR approach can be used for installing prplMesh as a package.

```bash
DESTDIR=/tmp/prplMesh-install ninja install
```

## Running Instructions

Once built, prplMesh controller, agent and framework can be started using `prplmesh_utils.sh`:

```bash
cd <path/to/install/dir>/scripts
sudo ./prplmesh_utils.sh start
```

## Debugging Instructions

To debug prplMesh controller, agent or cli it is needed to install 'Native Debug'
extension for Visual Studio Code. Also prplMesh solution should be compiled with
`CMAKE_BUILD_TYPE=Debug` flag.
Debug instruction: 
1. Start prplMesh solution(read Running Instructions)
2. To remote debug it is needed to start gdbserver
```bash
gdbserver :9999 --attach <pid of controller/agent/cli>
```
2. Go to debug tab in the VSCode and choose one the option from the dropdown list.
3. Add breakpoint and click start debugging.

### Log files locations

- framework `/tmp/mapf`
- controller `/tmp/beerocks/logs/beerocks_controller.log`
- platform manager & backhaul manager `/tmp/beerocks/logs/beerocks_backhaul.log`
- agent `/tmp/beerocks/logs/beerocks_agent.log`
- agent fronthaul ap manager wlan0  `/tmp/beerocks/logs/beerocks_ap_manager_wlan0.log`
- agent fronthaul ap monitor wlan0  `/tmp/beerocks/logs/beerocks_monitor_wlan0.log`
- agent fronthaul ap manager wlan2  `/tmp/beerocks/logs/beerocks_ap_manager_wlan2.log`
- agent fronthaul ap monitor wlan2  `/tmp/beerocks/logs/beerocks_monitor_wlan2.log`

### Checking status

System is operational if you see `FSM: CONNECTED --> OPERATIONAL` in the main agent log. In the future there will be a bml cli command to verify operational state.

### Displaying the connection map (GUI)

There is a tool to display the connection map on a GUI in `tools/beerocks_analyzer`.
Its [README file](tools/beerocks_analyzer/README.md) explains how to use it.

### Troubleshooting

If master branch does not work / does not pass tests on your computer make sure that:

- you loaded ebtables kernel module: `sudo modprobe ebtables`
- you updated your docker images with `tools/docker/image-build.sh`
