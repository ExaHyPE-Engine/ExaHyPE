import os
import sys

sys.path.insert(0, os.path.abspath(".."))

from Elastic import Elastic

if __name__ == "__main__":
    elastic = Elastic()

    my_parser = elastic.setup_parser()
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveADERDG",
        dimensions=2,
        end_time=3.0,
        width=[20.0, 20.0, 20.0],
        offset=[-5.0, 0.0, 0.0],
        min_depth=4,
        amr_levels=0,
        patch_size=16,
        rk_order = 2,
        dg_order = 3,
        number_of_snapshots = 20,
        peano_dir="../../../../",
    )
    my_args = my_parser.parse_args()

    my_solver = elastic.setup_solver(my_args)
    my_solver.add_user_solver_includes(
        """
#include "../Elastic.h"
"""
    )

    my_project = elastic.setup_project(my_args, my_solver)

    my_project.constants.define_value("RHO", str(2.7))
    my_project.constants.define_value("P_WAVE_SPEED", str(6.0))
    my_project.constants.define_value("S_WAVE_SPEED", str(3.464))

    my_project.output.makefile.add_cpp_file("Elastic.cpp")

    elastic.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
