import os
import sys

sys.path.insert(0, os.path.abspath("../../../../applications/exahype2/swe"))

from SWE import SWE

if __name__ == "__main__":
    swe = SWE()

    my_parser = swe.setup_parser()
    my_parser.set_defaults(
        solver="FWaveRiemannGlobalAdaptiveEnclaveFV",
        end_time=50.0,
        width="[10000, 10000]",  # [m]
        offset="[-5000, -5000]",  # [m]
        min_depth=5,
        amr_levels=0,
        patch_size=4,
        stateless=True,
        number_of_snapshots=0,
        peano_dir="../../../../",
    )
    my_args = my_parser.parse_args()

    my_solver = swe.setup_solver(my_args)
    my_solver.add_user_solver_includes(
        """
#include "../../../../applications/exahype2/swe/SWE.h"
#include "../../../../applications/exahype2/swe/FWave.h"
#include "../../../../applications/exahype2/swe/HLLEM.h"
#include "../../../../applications/exahype2/swe/Rusanov.h"
"""
    )

    my_project = swe.setup_project(my_args, my_solver)

    my_project.constants.export_constexpr_with_type(
        "DRY_TOLERANCE", str(1e-2), "double"
    )
    my_project.constants.export_constexpr_with_type(
        "INITIAL_WATER_HEIGHT", str(100.0), "double"
    )
    my_project.constants.export_constexpr_with_type(
        "INITIAL_BATHYMETRY_BEFORE_EARTHQUAKE", str(-100.0), "double"
    )

    my_project.output.makefile.add_cpp_file("SWE.cpp")
    my_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/swe/SWE.cpp"
    )
    my_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/swe/FWave.cpp"
    )
    my_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/swe/HLLEM.cpp"
    )
    my_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/swe/Rusanov.cpp"
    )

    swe.build(my_args, my_project)

    print(my_args)
    print(my_solver)
    print(my_project)
