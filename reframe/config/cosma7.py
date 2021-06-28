site_configuration = {
    'systems': [
        {
            'name': 'cosma7',
            'descr': 'cosma7',
            'hostnames': ['login7b.pri.cosma7.alces.network', 'login7a.pri.cosma7.alces.network', 'login7c.pri.cosma7.alces.network'],
            'modules_system': 'tmod4',
            'partitions': [
                {
                    'name': 'login',
                    'descr': 'login nodes',
                    'scheduler': 'local',
                    'launcher': 'local',
                    'environs': ['intel'],
                    'max_jobs': 100,
                },
                {
                    'name': 'multi_ranks_multi_node',
                    'descr': 'parallel jobs',
                    'scheduler': 'slurm',
                    'launcher': 'mpirun',
                    'access': ['-p cosma7', '-A durham'],
                    'environs': ['intel'],
                    'max_jobs': 100,
                },
            ]
        }
    ],
    'environments': [
        {
            'modules': ['gnu_comp/9.3.0',
                        'python/3.6.5',
                        'openmpi/4.0.3'],
             'name': 'intel',
             'cc': '',
             'cxx': 'g++',
             'ftn': '',
             'target_systems': ['cosma7'],
             'variables': [
                 ['PYTHONPATH','../../../python'],
                 ['OMP_NUM_THREADS','4'],
             ],
        },
    ],
    'logging': [
        {
            'level': 'debug',
            'handlers': [
                {
                    'type': 'stream',
                    'name': 'stdout',
                    'level': 'info',
                    'format': '%(message)s'
                },
                {
                    'type': 'file',
                    'name': 'reframe.log',
                    'level': 'debug',
                    'format': '[%(asctime)s] %(levelname)s: %(check_info)s: %(message)s',   # noqa: E501
                    'append': False
                }
            ],
            'handlers_perflog': [
                {
                    'type': 'filelog',
                    'prefix': '%(check_system)s/%(check_partition)s',
                    'level': 'info',
                    'format': (
                        '%(check_job_completion_time)s|reframe %(version)s|'
                        '%(check_info)s|jobid=%(check_jobid)s|'
                        '%(check_perf_var)s=%(check_perf_value)s|'
                        'ref=%(check_perf_ref)s '
                        '(l=%(check_perf_lower_thres)s, '
                        'u=%(check_perf_upper_thres)s)|'
                        '%(check_perf_unit)s'
                    ),
                    'append': True
                }
            ]
        }
    ],
}
