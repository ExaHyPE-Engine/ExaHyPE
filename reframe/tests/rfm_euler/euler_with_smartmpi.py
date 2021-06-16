import os
import reframe as rfm
import reframe.core.launchers.mpi
import reframe.utility.sanity as sn
from reframe.core.backends import getlauncher

import common


GIT_REV = os.environ['GIT_REVISION']

@rfm.parameterized_test(*([git_rev, topology]
    for git_rev in [GIT_REV]
    for topology in [
                '', # first value is empty for no SmartMPI.
                'Merged',
                'OneToOne',
                'Alternating',
                'NToOneServerAlt'
    ]
))
class Euler_with_smartmpi_CI(rfm.RegressionTest):
    def __init__(self, git_rev, topology):
        
        common.setup(self, num_tasks=4, num_cpus_per_task=4) # 4 ranks here means the domain decomposition fails

        self.time_limit = '10m'
        
        self.valid_systems = [
                'hamilton:multi_ranks_multi_node',
                'dine:multi_ranks_multi_node',
        ]

        self.test_dir = 'Peano/examples/exahype2/euler'
        
        # Add smartmpi to the prebuild commands:
        self.prebuild_cmds = [
                'rm -rf smartmpi',
                'git clone https://gitlab.lrz.de/hpcsoftware/smartmpi.git',
                'pushd smartmpi',
                'git checkout master; git pull',
                'libtoolize; aclocal; autoconf; autoheader',
                'cp src/config.h.in .',
                'automake --add-missing',
                './configure --with-mpi=mpiicpc CXXFLAGS="-std=c++17"', # CXX=mpicxx 
                'make -j',
                'popd',
        ] + self.prebuild_cmds
        
        # config options for Peano with Smartmpi and OneToOne topology
        self.build_system.config_opts = [
                '--enable-blockstructured',
                '--enable-exahype',
                '--enable-loadbalancing',
                '--with-multithreading=omp',
                f'--with-smartmpi={topology}',
                'LDFLAGS=-L$PWD/../smartmpi/src',
        ]

        if self.current_system.name == 'dine':
            self.build_system.config_opts += [
                    'CXXFLAGS="-fopenmp -std=c++14 -DnoMPISupportsSingleSidedCommunication -I$PWD/../smartmpi/src"',
                    '--with-mpi=mpicxx',
            ]
        elif self.current_system.name == 'hamilton':
            self.build_system.config_opts += [
                'CXXFLAGS="-fopenmp -std=c++14 -I$PWD/../smartmpi/src"',
                '--with-mpi=mpiicpc',
            ]
        
        self.keep_files = [
                f'{self.test_dir}/*.peano-patch-file',
                f'{self.test_dir}/exahype.log-filter',
        ]

        self.prerun_cmds = [
                f'pushd {self.test_dir}',
                'python3 example-scripts/finitevolumes.py -cs 0.1 -f --no-compile -t default -et 0.0001 -pdt 0.0001 -m debug',
                'make -j',
        ]
        
        self.executable = './peano4'


    @run_after('run')
    def check_patch_files(self):
        for f in os.listdir(os.path.join(self.stagedir, self.test_dir)):
            if '.peano-patch-file' in f:
                sn.evaluate(sn.assert_not_found(r'NaN', os.path.join(self.stagedir, self.test_dir, f)))

