import os
import shutil
from git import Repo
import touch

import reframe as rfm
import reframe.core.launchers.mpi
from reframe.core.backends import getlauncher

import common

GIT_REV = os.environ["GIT_REVISION"]


"""
This class ONLY builds the peano core for a given git commit hash. We use this so we 
don't need to build the core for every test case
"""

@rfm.parameterized_test(*([git_rev]
    for git_rev in [GIT_REV]
))
class Build_peano_core(rfm.CompileOnlyRegressionTest):
    def __init__(self, git_rev):

        # 4 ranks here means the domain decomposition fails
        common.setup(self, git_rev, num_tasks=1, num_cpus_per_task=4)

        self.time_limit = "5m"

        self.valid_systems = [
            "hamilton:local_launcher",
            "dine:local_launcher",
        ]
        
        self.keep_files = ["Peano"]

        self.build_system.config_opts = [
            "--enable-blockstructured",
            "--enable-exahype",
            "--enable-loadbalancing",
            "--with-multithreading=omp",
        ]

        if self.current_system.name == "dine":
            self.build_system.config_opts += [
                'CXXFLAGS="-fopenmp -std=c++17 -DnoMPISupportsSingleSidedCommunication"',
                "--with-mpi=mpicxx",
            ]
        elif self.current_system.name == "hamilton":
            self.build_system.config_opts += [
                'CXXFLAGS="-fopenmp -std=c++17"',
                "--with-mpi=mpiicpc",
            ]

    @run_after('compile')
    def remove_files_obstructing_copying_of_peano(self):
        # We have to delete the .git/ folder else ReFrame fails to copy Peano to the output dir
        try:
            shutil.rmtree(os.path.join(str(self.stagedir), 'Peano', '.git'))
        except OSError as e:
            print("Error: %s - %s." % (e.filename, e.strerror))
