import os
from git import Repo

import reframe as rfm
import reframe.core.launchers.mpi
import reframe.utility.sanity as sn
from reframe.core.backends import getlauncher

import common
from post_processing import getTimeStepping


GIT_REV = os.environ["GIT_REVISION"]


@rfm.parameterized_test(*([git_rev] for git_rev in [GIT_REV]))
class Euler_CI(rfm.RegressionTest):
    def __init__(self, git_rev):

        # 4 ranks here means the domain decomposition fails
        common.setup(self, git_rev, num_tasks=1, num_cpus_per_task=4)

        self.time_limit = "5m"

        self.valid_systems = [
            "hamilton:multi_ranks_multi_node",
            "dine:multi_ranks_multi_node",
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
            f"{self.test_dir}/*.peano-patch-file",
            f"{self.test_dir}/exahype.log-filter",
        ]

        self.prerun_cmds = [
            f"pushd {self.test_dir}",
            "python3 example-scripts/finitevolumes.py -cs 0.1 -f --no-compile -t default -et 0.0001 -pdt 0.0001",
            "make -j",
        ]

        self.executable = "./peano4"

    """
    Simple sanity check for NaNs in patch files
    """

    @run_after("run")
    def check_patch_files(self):
        for f in os.listdir(os.path.join(self.stagedir, self.test_dir)):
            if ".peano-patch-file" in f:
                sn.evaluate(
                    sn.assert_not_found(
                        r"NaN", os.path.join(self.stagedir, self.test_dir, f)
                    )
                )

    """
    Compare performance with that of previous commit for which data exists.
    If performance has degraded then test fails and follow up with a profiler
    is triggered. If performance radically increases test also fails as this
    likely indicates the introduction of flawed code: nothing in life is too
    good to be true ;)
    """

    @run_after("run")
    def compare_with_previous_performance(self):
        repo = Repo(search_parent_directories=True)

        for commit in list(repo.iter_commits(max_count=100)):
            hexsha = commit.hexsha

            if hexsha == self.git_rev:
                continue

            for dir_name in os.listdir(os.path.join(str(self.outputdir), "..")):
                target_file = os.path.join(
                    str(self.outputdir),
                    "..",
                    dir_name,
                    "rfm_Euler_CI_{self.git_rev}_job.out",
                )
                if hexsha in dir_name and os.path.exists(target_file):
                    print(f"Comparing run times of {self.git_rev} and {hexsha}")

                    time_prev_commit = getTimeStepping(target_file)

                    time_this_commit = getTimeStepping(
                        os.path.join(str(self.stagedir), str(self.stdout))
                    )

                    if time_prev_commit < (0.9 * time_this_commit):
                        # Set var that triggers follow up with profiler
                        os.environ["PERF_DROP"] = "True"
                        # Fail the test:
                        sn.evaluate(
                            sn.assert_lt(
                                time_prev_commit,
                                (0.9 * time_this_commit),
                                "Performance degredation has been detected",
                            )
                        )

                    # if performance increases suspiciously we fail the test:
                    sn.evaluate(
                        sn.assert_gt(
                            time_prev_commit,
                            (1.1 * time_this_commit),
                            "Performance increase is cause for suspicion",
                        )
                    )

                    return

        print(f"Nothing found to compare with this test")
