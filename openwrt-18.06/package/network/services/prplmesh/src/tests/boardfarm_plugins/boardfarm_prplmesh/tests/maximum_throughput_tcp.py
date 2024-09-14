# SPDX-License-Identifier: BSD-2-Clause-Patent
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

from .prplmesh_base_test import PrplMeshBaseTest
from boardfarm.exceptions import SkipTest
from datetime import datetime
from opts import debug

import threading


class MaximumThroughputTcp(PrplMeshBaseTest):
    """Check initial configuration on device."""

    def runTest(self):
        try:
            agent = self.dev.DUT.agent_entity
            remote_sta = self.get_device_by_name('remote_station')

        except AttributeError as ae:
            raise SkipTest(ae)

        results_24 = {'downlink_2.4': [0],
                      'uplink_2.4': [0]}

        results_5 = {'downlink_5': [0],
                     'uplink_5': [0]}

        cpu_info = []
        test_location = []
        test_location.append('pre-conditions')

        try:
            stop_event = threading.Event()
            thread_1 = threading.Thread(target=self.get_cpe_resources,
                                        args=[agent, cpu_info, test_location, stop_event])
            thread_1.start()
            # Preconditions TODO
            # - 2.4GHz band
            #     ->Channel - 6
            #     ->Standard - 802.11n
            #     ->Channel bandwidth - 20MHz
            # - 5GHz band
            #     -> Channel - 36 (non-DFS)
            #     -> Standard - 802.11ac
            #     -> Channel bandwidth - 80MHz

            self.configure_passphrase()
            # Establish the connection and allow STA to associate with the DUT. 2.4
            # Check if the STA is connected to the right BSSID using the right capabilites

            result = remote_sta.wifi_connect_check(agent.radios[0].vaps[0])

            if not result:
                self.fail(f'Connection status: {result}')
            # Measure the STA downlink TCP throughput using a test time
            # of 120 seconds and repeat the 3 times.
            # duration=120, num_streams=10)

            for iteration in range(1, 4):
                test_location.append(f'Down 2.4 - {iteration}')
                tp = remote_sta.iperf_throughput(
                    to_dut=True, protocol='tcp', duration=120, num_streams=10)
                results_24['downlink_2.4'].append(tp)
            # Measure the STA uplink TCP throughput using a test time
            #  of 120 seconds and repeat the 3 times.
            # duration=120, num_streams=10)
                test_location.append(f'Up 2.4 - {iteration}')
                tp = remote_sta.iperf_throughput(
                    to_dut=False, protocol='tcp', duration=120, num_streams=10)
                results_24['uplink_2.4'].append(tp)

            # pass criteria 90 down / 90 up
            test_location.append('Connecting to 5GHz')

            remote_sta.wifi_disconnect(agent.radios[0].vaps[0])

            # Establish the connection and allow STA to associate with the DUT. 5
            # Check if the STA is connected to the right BSSID using the right capabilites
            result = remote_sta.wifi_connect_check(agent.radios[1].vaps[0])

            if not result:
                self.fail(f'Connection status: {result}')

            # Measure the STA downlink TCP throughput using a test time
            #  of 120 seconds and repeat the 3 times.

            for iteration in range(1, 4):
                test_location.append(f'Down 5 - {iteration}')
                tp = remote_sta.iperf_throughput(
                    to_dut=True, protocol='tcp', duration=120, num_streams=10)
                results_5['downlink_5'].append(tp)

            # Measure the STA uplink TCP throughput using a test time
            #  of 120 seconds and repeat the 3 times.
                test_location.append(f'Up 5 - {iteration}')
                tp = remote_sta.iperf_throughput(
                    to_dut=False, protocol='tcp', duration=120, num_streams=10)
                results_5['uplink_5'].append(tp)

            # pass criteria 504 down / 504 up

        finally:
            stop_event.set()
            self.print_cpe_stats(cpu_info, results_24, results_5)

    def get_cpe_resources(self, agent, cpu_info, test_location, stop_event):
        while not stop_event.wait(4):
            memory_stats = agent.get_memory_usage()

            cpu_stats = agent.get_cpu_usage()

            cpu_info.append({'time': str(datetime.now()),
                             'location': test_location[-1],
                             'stats': {
                'cpu_stats': cpu_stats,
                'memory_stats': memory_stats,
            }
            })

    def print_cpe_stats(self, cpu_info, results_24, results_5):
        # Header row
        debug(f'{" ": <20}', end='')
        for key, named_tuple in cpu_info[0]['stats'].items():
            for name, stat in named_tuple._asdict().items():
                debug(f'| {name: <12} |', end='')

        for info in cpu_info:
            debug(f'\n{info["time"].split(".")[0]} ', end='')
            for key, named_tuple in info['stats'].items():
                for name, stat in named_tuple._asdict().items():
                    debug(f'| {stat: < 12} |', end='')
            debug(f'| {info["location"]} ', end='')

        debug(f"\n\nMax download 2.4GHz {max(results_24['downlink_2.4'])}")
        debug(f"Max upload 2.5GHz {max(results_24['uplink_2.4'])}")

        debug(f"\nMax download 5GHz {max(results_5['downlink_5'])}")
        debug(f"Max upload 5GHz {max(results_5['uplink_5'])}")
