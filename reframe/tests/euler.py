import os
import reframe as rfm
import reframe.core.launchers.mpi

import common


GIT_REV = os.environ['GIT_REVISION']

@rfm.parameterized_test(*([git_rev]
    for git_rev in [GIT_REV]
))
class Euler_CI(rfm.RegressionTest):
    def __init__(self, git_rev):
        
        common.setup(self, num_tasks=4, num_cpus_per_task=6)

        self.time_limit = '2h'
        
        self.valid_systems = ['hamilton:multi_ranks_multi_node']

        test_dir = 'Peano/examples/exahype2/euler'
        
        self.build_system.config_opts = [
                '--enable-blockstructured',
                '--enable-exahype',
                '--enable-loadbalancing',
                '--with-mpi=mpiicpc',
                '--with-multithreading=omp',
                'CXXFLAGS="-fopenmp -std=c++14"',
        ]
        
        self.prerun_cmds = [
                'module load python/3.6.8',
                f'pushd {test_dir}',
                'python3 example-scripts/finitevolumes.py -cs 0.1 -f --no-compile -t enclave-ats -et 0.0001 -m debug',
                'make',
        ]
        
        self.executable = './peano4'

