import os

import reframe.utility.sanity as sn
import reframe as rfm

def setup(test, num_tasks, num_cpus_per_task=1):
    test.time_limit = '2d'
    test.num_tasks = num_tasks
    test.num_cpus_per_task = num_cpus_per_task

    test.sourcesdir = None
    git_rev = os.environ['GIT_REVISION']
    test.prebuild_cmds = [
        'rm -rf Peano',
        'git clone https://gitlab.lrz.de/hpcsoftware/Peano.git',
        'pushd Peano',
        f'git checkout {git_rev}',
        'git clean -x -f -d',
        'libtoolize; aclocal; autoconf; autoheader',
        'cp src/config.h.in .',
        'automake --add-missing',
    ]
    test.build_system = 'Autotools'
    test.sourcepath = './'
    test.build_system.max_concurrency = 32

    test.sanity_patterns = sn.assert_found(r'Peano 4 terminated successfully.', test.stdout)

    test.valid_prog_environs = ['intel', 'amd']
