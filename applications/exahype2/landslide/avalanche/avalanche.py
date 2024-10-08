import os
import sys

sys.path.insert(0, os.path.abspath(".."))

from landslide import Landslide

if __name__ == "__main__":
    landslide = Landslide()

    my_parser = landslide.setup_parser()
    my_parser.set_defaults(
        solver="RusanovGlobalAdaptiveFV",
        end_time=1.0,
        width=[1.58, 0.70],
        offset=[0.0, 0.0],
        min_depth=2,
        amr_levels=4,
        patch_size=16,
        number_of_snapshots=100,
        peano_dir="../../../../",
    )
    my_args = my_parser.parse_args()

    my_solver = landslide.setup_solver(my_args)
    my_solver.add_user_solver_includes(
        """
#include "../Landslide.h"
#include "../ExtrapolateHalo.h"
#include "../ComputePartialDerivatives.h"
"""
    )

    my_project = landslide.setup_project(my_args, my_solver)

    my_project.output.makefile.add_cpp_file("Landslide.cpp")

    landslide.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
