#!/usr/bin/env python3
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
#
# This code is subject to the terms of the BSD+Patent license.
#
# See LICENSE file for more details.
#
# Launch the test suite using docker and docker-compose. This script wraps
# the creation of the bridge(s) to be able to connect external devices with
# the docker network, launching the service for boardfarm.
#
# As this script is run outside containers, it does not use anything apart
# from Python 3.5 (will work on later versions but only uses 3.5 features)
#
# The best way to make sure no Python 3.5+ features are used is running the
# script with a Python 3.5.0 interpreter. Compile it from:
#
# https://www.python.org/ftp/python/3.5.0/Python-3.5.0.tgz
#
# Also, when calling a function look for 'New in version 3.X' where X > 5
#
from __future__ import print_function  # To check for python2 or < 3.5 execution
import argparse
import os
import sys
import json
from subprocess import Popen, PIPE, run

if not (sys.version_info.major == 3 and sys.version_info.minor >= 5):
    print("This script requires Python 3.5 or higher!")
    print("You are using Python {}.{}.".format(sys.version_info.major, sys.version_info.minor))
    sys.exit(1)


def check_docker_versions():
    DOCKER_MAJOR = 19
    DC_MAJOR = 1
    DC_MINOR = 25
    docker_version = os.popen('docker --version').read().split(' ')[2]
    docker_major = int(docker_version.split('.')[0])
    if docker_major < DOCKER_MAJOR:
        fmt = "This script requires docker {}.0 or higher"
        print(fmt.format(DOCKER_MAJOR))
        print("You are usng version {}".format(docker_version))
        sys.exit(1)
    dc_version = os.popen('docker-compose --version').read().split(' ')[2]
    dc_major = int(dc_version.split('.')[0])
    dc_minor = int(dc_version.split('.')[1])
    if dc_major < DC_MAJOR:
        fmt = "This script requires docker-compose {}.{} or higher"
        print(fmt.format(DC_MAJOR, DC_MINOR))
        print("You are usng version {}".format(dc_version))
        sys.exit(1)
    if dc_minor < DC_MINOR:
        fmt = "This script requires docker-compose {}.{} or higher"
        print(fmt.format(DC_MAJOR, DC_MINOR))
        print("You are usng version {}".format(dc_version))
        sys.exit(1)


class Services:
    def __init__(self, dut, test_suite, bid=None):
        self.scriptdir = os.path.dirname(os.path.realpath(__file__))
        os.chdir(self.scriptdir)
        self.rootdir = self.scriptdir
        self.dut = dut
        self.test_suite = test_suite

        if bid is not None:
            self.build_id = bid
            print('Using ID {}'.format(self.build_id))
            # return
        else:
            self.build_id = self.get_build_id()

        self.logdir = os.path.join(self.scriptdir, 'logs')
        if not os.path.exists(self.logdir):
            os.makedirs(self.logdir)
        # dumpcap needs dir to be writable by anyone since it drops root capabilities
        # (specifically CAP_FOWNER) after opening the network if and cannot write the dump.
        os.chmod(self.logdir, 0o777)

        for device in self._get_device_names():
            device_name = '{}-{}'.format(device, self.build_id)
            devicedir = os.path.join(self.logdir, device_name)
            if not os.path.exists(devicedir):
                print('Making {}'.format(devicedir))
                os.makedirs(devicedir)

    def _get_device_names(self):
        jspath = './tests/boardfarm_plugins/boardfarm_prplmesh/prplmesh_config.json'
        js = json.loads(open(jspath, 'r').read())
        devices = [js[self.dut]['name']]
        for device in js[self.dut]['devices']:
            devices.append(device['name'])
        return devices

    def get_build_id(self):
        ci_pipeline_id = os.getenv('CI_PIPELINE_ID')
        if ci_pipeline_id is not None:
            return ci_pipeline_id

        # Otherwise we are running on the local machine, just find last id
        # created and add one
        last_id = 0
        if not os.path.exists('logs'):
            return str(1)

        # Search if a directory exists with logs/<device>-<X> and use X+1 as
        # id. Get the first device from the json list
        search_prefix = self._get_device_names()[0] + '-'
        for d in os.listdir('logs'):
            if d.startswith(search_prefix):
                suffix = d[len(search_prefix):]
                try:
                    isuffix = int(suffix)
                except ValueError:
                    isuffix = 0
                if isuffix > last_id:
                    last_id = isuffix
        if last_id == 0:
            new_id = 1
        else:
            new_id = last_id + 1
        return str(new_id)

    def dc(self, args, interactive=False):
        params = ['docker-compose', '-f',
                  'tools/docker/boardfarm-ci/docker-compose.yml']
        params += args
        local_env = os.environ
        local_env['ROOT_DIR'] = self.rootdir
        local_env['RUN_ID'] = self.build_id
        if os.getenv('PARENT_PIPELINE_ID'):
            # Running from a child pipeline. Use the parent pipeline ID:
            local_env['IMAGE_TAG'] = local_env['PARENT_PIPELINE_ID']
        elif os.getenv('CI_PIPELINE_ID'):
            # Running from the main pipeline:
            local_env['IMAGE_TAG'] = local_env['CI_PIPELINE_ID']
        else:
            # Running locally
            local_env['IMAGE_TAG'] = 'latest'

        print("Using IMAGE_TAG '{}'".format(local_env['IMAGE_TAG']))

        local_env['FINAL_ROOT_DIR'] = self.rootdir

        local_env['DUT'] = self.dut
        local_env['TEST_SUITE'] = self.test_suite

        if not interactive:
            proc = Popen(params, stdout=PIPE, stderr=PIPE)
            for line in proc.stdout:
                print(line.decode(), end='')
            proc.stdout.close()
        else:
            proc = Popen(params)
        return_code = proc.wait()
        return return_code


def cleanup(rc):
    if rc != 0:
        print('Return code !=0 -> {}'.format(rc))
    sys.exit(rc)


if __name__ == '__main__':
    check_docker_versions()
    parser = argparse.ArgumentParser(description='Dockerized test launcher')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--clean', dest='clean', action='store_true',
                       help='Clean containers images and networks')
    group.add_argument('--shell', dest='shell', action='store_true',
                       help='Run a shell on the bf container')
    group.add_argument('--comp', dest='comp', action='store_true',
                       help='Pass the rest of arguments to docker-compose')
    parser.add_argument('--id', dest='bid', type=str,
                        help='Specify the id to use for build/shell/comp/clean')
    parser.add_argument('--dut', dest='dut', type=str, help='Device under test',
                        default='prplmesh_compose')

    test_group = parser.add_mutually_exclusive_group()
    test_group.add_argument('--test', dest='test', type=str,
                            help='Comma-separated list of individual tests to run')
    test_group.add_argument('--test-suite', dest='test_suite', type=str,
                            help='Test suite to be run')

    args, rest = parser.parse_known_args()

    if args.test is not None:
        # We abuse `test_suite` argument to contain a list of tests.
        # This value will be parsed in `run_bf.sh`.
        args.test_suite = "TEST_LIST:" + args.test
    elif args.test_suite is None:
        args.test_suite = "test_flows"

    if os.getenv('CI_PIPELINE_ID') is not None:
        args.bid == os.getenv('CI_PIPELINE_ID')

    if args.comp:
        if args.bid is None:
            print('Specify --id for the --comp parameter')
            sys.exit(0)
        services = Services(dut=args.dut, test_suite=args.test_suite, bid=args.bid)
        if len(rest) == 0:
            print('Usage: dctest --id <id> --comp <arguments to docker-compose>')
            sys.exit(1)
        sys.exit(services.dc(rest, interactive=True))
    else:
        if len(rest) > 0:
            print('Unknown parameters: {}'.format(rest))
            sys.exit(1)

    if args.clean:
        if args.bid is None:
            print('Specify --id for the --clean parameter')
            sys.exit(0)
        services = Services(dut=args.dut, test_suite=args.test_suite, bid=args.bid)
        rc = services.dc(['down', '--remove-orphans'])
        network_names_cmd = ["docker", "network", "ls", "-q", "--filter",
                             r"name=^(\d+-)?prplMesh-net-.*"]
        network_names = run(network_names_cmd,
                            check=True, capture_output=True).stdout.decode("utf-8").splitlines()
        print("Removing networks (if unused): {}".format(str(network_names)))
        network_rm_cmd = ["docker", "network", "rm"] + network_names
        run(network_rm_cmd, check=False, capture_output=True)

        cleanup(rc)
    elif args.shell:
        if not args.bid:
            print('Specify --id for the shell parameter')
            sys.exit(0)
        services = Services(dut=args.dut, test_suite=args.test_suite, bid=args.bid)
        rc = services.dc(['run', '--rm', '--service-ports', '--entrypoint',
                          '/bin/bash', 'boardfarm'], interactive=True)
        cleanup(rc)
    else:
        if args.bid:
            services = Services(dut=args.dut, test_suite=args.test_suite, bid=args.bid)
        else:
            services = Services(dut=args.dut, test_suite=args.test_suite)   # With new build id
        try:
            rc = services.dc(['up', '--exit-code-from', 'boardfarm', '--abort-on-container-exit'],
                             interactive=True)
        finally:
            services.dc(['down'])
        cleanup(rc)
