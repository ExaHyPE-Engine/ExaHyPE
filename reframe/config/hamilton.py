site_configuration = {
    "systems": [
        {
            "name": "hamilton",
            "descr": "Hamilton ARC supercomputer",
            "hostnames": ["hamilton1.hpc.dur.ac.uk", "hamilton2.hpc.dur.ac.uk"],
            "modules_system": "tmod",
            "partitions": [
                {
                    "name": "login",
                    "descr": "login nodes",
                    "scheduler": "local",
                    "launcher": "local",
                    "environs": ["intel"],
                    "max_jobs": 100,
                },
                {
                    "name": "local_launcher",
                    "descr": "local launcher",
                    "scheduler": "slurm",
                    "launcher": "local",
                    "access": ["-p par6.q"],
                    "environs": ["intel"],
                    "max_jobs": 100,
                },

                {
                    "name": "multi_ranks_multi_node",
                    "descr": "parallel jobs (Hamilton7) - single node",
                    "scheduler": "slurm",
                    "launcher": "mpirun",
                    "access": ["-p par7.q"],
                    "environs": ["intel"],
                    "max_jobs": 30,
                },
            ],
        }
    ],
    "environments": [
        {
            "modules": ["gsl/intel/2.4", "intel/2020.4", "gcc/9.3.0", "intelmpi/intel/2019.6", "python/3.6.8"], # intel/2019.5 is not a suitable replacement for intel/2020.4. Note also: this setup loads gcc/8.2.0 by default - this must be manually unloaded in the test case.
            "name": "intel",
            "cc": "icc",
            "cxx": "icpc",
            "ftn": "",
            "target_systems": ["hamilton"],
            "variables": [
                ["PYTHONPATH", "../../../python"],
                ["OMP_NUM_THREADS", "4"],
            ],
        }
    ],
    "logging": [
        {
            "level": "debug",
            "handlers": [
                {
                    "type": "stream",
                    "name": "stdout",
                    "level": "info",
                    "format": "%(message)s",
                },
                {
                    "type": "file",
                    "name": "reframe.log",
                    "level": "debug",
                    "format": "[%(asctime)s] %(levelname)s: %(check_info)s: %(message)s",  # noqa: E501
                    "append": False,
                },
            ],
            "handlers_perflog": [
                {
                    "type": "filelog",
                    "prefix": "%(check_system)s/%(check_partition)s",
                    "level": "info",
                    "format": (
                        "%(check_job_completion_time)s|reframe %(version)s|"
                        "%(check_info)s|jobid=%(check_jobid)s|"
                        "%(check_perf_var)s=%(check_perf_value)s|"
                        "ref=%(check_perf_ref)s "
                        "(l=%(check_perf_lower_thres)s, "
                        "u=%(check_perf_upper_thres)s)|"
                        "%(check_perf_unit)s"
                    ),
                    "append": True,
                }
            ],
        }
    ],
}
