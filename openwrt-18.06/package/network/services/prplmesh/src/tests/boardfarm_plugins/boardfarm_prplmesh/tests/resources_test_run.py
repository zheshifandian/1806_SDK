from .prplmesh_base_test import PrplMeshBaseTest
from opts import debug


class ResourcesTestRun(PrplMeshBaseTest):
    """Check initial configuration on device."""

    def runTest(self):

        agent = self.dev.DUT.agent_entity

        memory_stats = agent.get_memory_usage()

        debug('memory_stats' + str(memory_stats))

        assert all([isinstance(v, int) for v in memory_stats])

        cpu_stats = agent.get_cpu_usage()

        debug('cpu_stats' + str(cpu_stats))

        assert all([isinstance(v, float) for v in cpu_stats])
