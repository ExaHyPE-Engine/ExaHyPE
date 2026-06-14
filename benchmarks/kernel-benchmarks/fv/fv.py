# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys

import exahype2

sys.path.insert(0, os.path.abspath("../../../tests/fv"))
from PDE import max_eigenvalue, flux

initial_conditions = """
  Q[Shortcuts::rho]       = 1.0;
  Q[Shortcuts::rhoU + 0]  = 0.0;
  Q[Shortcuts::rhoU + 1]  = 0.0;
#if DIMENSIONS == 2
  Q[Shortcuts::rhoE]      = ((sqrt(pow(0.5 - x(0), 2) + pow(0.5 - x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[Shortcuts::rhoU + 2]  = 0.0;
  Q[Shortcuts::rhoE]      = ((sqrt(pow(0.5 - x(0), 2) + pow(0.5 - x(1), 2) + pow(0.5 - x(2), 2)) < 0.2) ? (1.0) : (1.01));
#endif
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoU + 2] = -Qinside[Shortcuts::rhoU + 2];
#endif
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=4)
    parser.add_argument(
        "-s",
        "--samples",
        type=int,
        default=0,
        help="Set number of samples for benchmarking, if not set, run until scenario end",
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

size = [1.0, 1.0, 1.0]

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarks",
    dimensions=2,
    size=size,
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="KernelBenchmarks",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
    use_enclave_tasking=True,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

project = project.generate_Peano4_project(verbose=False)

project.constants.export_constexpr_with_type(
    "BenchmarkMinTerminalTime", str(args.end_time), "double"
)
project.constants.export_constexpr_with_type(
    "BenchmarkMaxTerminalTime", str(args.end_time), "double"
)
project.constants.export_constexpr_with_type(
    "BenchmarkGridLevels", str(args.grid_levels), "int"
)
project.constants.export_constexpr_with_type(
    "BenchmarkMaxGridLevels", str(args.grid_levels), "int"
)

project.constants.export_constexpr_with_type("ShouldPlot", "true", "bool")

project.constants.export_constexpr_with_type("UseAMR", "false", "bool")

project.constants.export_constexpr_with_type("Samples", str(args.samples), "int")

makefile = project.output.makefile
makefile.add_cpp_file("KernelBenchmarks-main.cpp")

makefile.add_h_file("DataRepository.h")
makefile.add_cpp_file("DataRepository.cpp")

makefile.add_h_file("EnclaveDataRepository.h")
makefile.add_cpp_file("EnclaveDataRepository.cpp")

makefile.add_CXX_flag("-DGAMMA=1.4")

project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
