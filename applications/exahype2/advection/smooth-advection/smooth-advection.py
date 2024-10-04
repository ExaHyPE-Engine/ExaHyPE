import os
import sys

sys.path.insert(0, os.path.abspath(".."))

from advection import Advection

if __name__ == "__main__":
    advection = Advection()

    my_parser = advection.setup_parser()
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveADERDG",
        dimensions=2,
        end_time=3.0,
        width="[1.0, 1.0, 1.0]",
        offset="[0.0, 0.0, 0.0]",
        periodic_boundary_conditions_x = True,
        min_depth=4,
        amr_levels=0,
        patch_size=16,
        rk_order = 2,
        dg_order = 3,
        number_of_snapshots = 20,
        peano_dir="../../../../",
    )
    my_args = my_parser.parse_args()

    my_solver = advection.setup_solver(my_args)
    my_solver.add_user_solver_includes(
        """
#include "../Advection.h"
"""
    )

    my_project = advection.setup_project(my_args, my_solver)

    my_project.constants.define_value("ADVECTION_SPEED", str(1.0))

    my_project.output.makefile.add_cpp_file("Advection.cpp")

    advection.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
