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
        
        common.setup(self, num_tasks=1, num_cpus_per_task=4) # 4 ranks here means the domain decomposition fails

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
        
        self.keep_files = [f'{test_dir}/*.peano-patch-file']

        self.prerun_cmds = [
                f'pushd {test_dir}',
                'python3 example-scripts/finitevolumes.py -cs 0.1 -f --no-compile -t default -et 0.0001 -pdt 0.0001',
                'make -j',
        ]
        
        self.executable = './peano4'

