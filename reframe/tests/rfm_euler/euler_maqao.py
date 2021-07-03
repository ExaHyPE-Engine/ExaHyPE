import os

import reframe as rfm
import reframe.core.launchers.mpi
import reframe.utility.sanity as sn
from reframe.core.backends import getlauncher

import common


GIT_REV = os.environ["GIT_REVISION"]


"""
Be sure to call this with --skip-sanity-check arg to ReFrame else 
the sanity checks set up by common.py will fail.
"""
@rfm.parameterized_test(*([git_rev] for git_rev in [GIT_REV]))
class Euler_maqao(rfm.RegressionTest):
    def __init__(self, git_rev):

        # 4 ranks here means the domain decomposition fails
        common.setup(self, git_rev, num_tasks=1, num_cpus_per_task=4)
        
        self.time_limit = "5m"

        self.valid_systems = [
            "dine:local_launcher_profiler_jobs",
        ]

        self.test_dir = "Peano/examples/exahype2/euler"

        self.build_system.config_opts = [
            "--enable-blockstructured",
            "--enable-exahype",
            "--enable-loadbalancing",
            "--with-multithreading=omp",
        ]

        if self.current_system.name == "dine":
            self.build_system.config_opts += [
                'CXXFLAGS="-fopenmp -std=c++14 -DnoMPISupportsSingleSidedCommunication"',
                "--with-mpi=mpicxx",
            ]
        elif self.current_system.name == "hamilton":
            self.build_system.config_opts += [
                'CXXFLAGS="-fopenmp -std=c++14"',
                "--with-mpi=mpiicpc",
            ]

        self.keep_files = [
            f"{self.test_dir}/maqao*",
        ]

        self.prerun_cmds = [
            f"pushd {self.test_dir}",
            "python3 example-scripts/finitevolumes.py -cs 0.1 -f --no-compile -t default -et 0.0001 -pdt 0.0001",
            "make -j",
        ]
        
        self.executable = "maqao"
        
        interface = ""
        if self.current_system.name == "dine":
            interface = "--mca btl_tcp_if_include p1p2 -x UCX_NET_DEVICES=mlx5_1:1"
        
        self.executable_opts = [
            'oneview',
            '--create-report=one',
            f'--mpi-command="mpirun -np {self.num_tasks} {interface}"',
            f'--omp-num-threads={self.num_cpus_per_task}',
            '-- ./peano4',
        ]

