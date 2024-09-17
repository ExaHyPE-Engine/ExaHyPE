import os
import sys

sys.path.insert(0, os.path.abspath(".."))

from acoustic import Acoustic

if __name__ == "__main__":
    acoustic = Acoustic()

    my_parser = acoustic.setup_parser()
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveFV",
        dimensions=2,
        end_time=3.0,
        width="[10.0, 10.0, 10.0]",
        offset="[0.0, 0.0, 0.0]",
        min_depth=1,
        amr_levels=5,
        patch_size=4,
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
