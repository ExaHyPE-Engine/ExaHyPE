import os

import reframe.utility.sanity as sn
import reframe as rfm

def build_setup(test, git_rev):
    test.sourcesdir = None
    test.git_rev = git_rev
    test.prebuild_cmds = [
        'rm -rf Peano',
        'git clone https://gitlab.lrz.de/hpcsoftware/Peano.git',
        'pushd Peano',
        f'git checkout {test.git_rev}',
        'git clean -x -f -d',
        'libtoolize; aclocal; autoconf; autoheader',
        'cp src/config.h.in .',
        'automake --add-missing',
    ]
    test.build_system = 'Autotools'
    test.sourcepath = './'
    test.build_system.max_concurrency = 32
    test.valid_prog_environs = ['intel', 'amd']


def setup(test, git_rev, num_tasks, num_cpus_per_task=1):
    test.time_limit = '2d'
    test.num_tasks = num_tasks
    test.num_cpus_per_task = num_cpus_per_task

    test.sourcesdir = None
    test.git_rev = git_rev
    test.prebuild_cmds = [
        'rm -rf Peano',
        'git clone https://gitlab.lrz.de/hpcsoftware/Peano.git',
        'pushd Peano',
        f'git checkout {test.git_rev}',
        'git clean -x -f -d',
        'libtoolize; aclocal; autoconf; autoheader',
        'cp src/config.h.in .',
        'automake --add-missing',
    ]
    test.build_system = 'Autotools'
    test.sourcepath = './'
    test.build_system.max_concurrency = 32
    
    # We can't add the normal sanity check when using the maqao profiler as Peano's
    # usual output won't be generated. For maqao we add a trivial pattern.
    if "maqao" not in test.name:
        test.sanity_patterns = sn.assert_found(r'Peano 4 terminated successfully.', test.stdout)
    else:
        test.sanity_patterns = sn.assert_eq(0, 0)
    
    test.valid_prog_environs = ['intel', 'amd']
