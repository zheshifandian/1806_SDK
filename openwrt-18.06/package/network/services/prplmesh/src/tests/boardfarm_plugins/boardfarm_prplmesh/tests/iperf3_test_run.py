from .prplmesh_base_test import PrplMeshBaseTest


class Iperf3TestRun(PrplMeshBaseTest):
    """Check initial configuration on device."""

    def runTest(self):

        agent = self.dev.DUT.agent_entity

        tp = agent.iperf_throughput(to_dut=True, protocol='tcp')
        if not tp:
            self.fail("Throughput test from boardfarm host to DUT failed: no results available.")

        tp = agent.iperf_throughput(to_dut=False, protocol='tcp')
        if not tp:
            self.fail("Throughput test from DUT to boardfarm host failed: no results available.")
