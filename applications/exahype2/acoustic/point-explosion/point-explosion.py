import os
import sys

sys.path.insert(0, os.path.abspath(".."))

from acoustic import Acoustic

if __name__ == "__main__":
    acoustic = Acoustic()

    my_parser = acoustic.setup_parser()
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveADERDG",
        dimensions=2,
        end_time=3.0,
        width="[10.0, 10.0, 10.0]",
        offset="[0.0, 0.0, 0.0]",
        min_depth=4,
        amr_levels=0,
        patch_size=16,
        rk_order = 2,
        dg_order = 3,
        number_of_snapshots = 20,
        peano_dir="../../../../",
    )
    my_args = my_parser.parse_args()

    my_solver = acoustic.setup_solver(my_args)
    my_solver.add_user_solver_includes(
        """
#include "../Acoustic.h"
"""
    )

    my_project = acoustic.setup_project(my_args, my_solver)

    my_project.constants.define_value("RHO", str(2.7))
    my_project.constants.define_value("WAVE_SPEED", str(6.0))
    my_project.constants.define_value("SIGMA", str(0.1149))

    my_project.output.makefile.add_cpp_file("Acoustic.cpp")

    acoustic.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
