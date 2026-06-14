# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

from numpy import pi

from GRMHD import max_eigenvalue, flux, ncp

# available_precisions = {
#     "bf16", "fp16", "fp32", "fp64"
# }

available_scenarios = ["AlfvenWave", "MichelSphericalAccretion"]


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=None)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=6)
    parser.add_argument("-l", "--grid-levels", type=int, default=1)
    # parser.add_argument("-pr", "--precision", default="fp64", choices=available_precisions)
    parser.add_argument(
        "-s", "--scenario", choices=available_scenarios, default="AlfvenWave"
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

order = args.degrees_of_freedom - 1

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

if args.scenario == "MichelSphericalAccretion":
    dimensions = 3
    offset = [2.2, 2.2, 2.2]
    size = [6.0, 6.0, 6.0]
    end_time = 1.0
    periodic_bc = False
    function_name = "initialAccretionDisc3D"
elif args.scenario == "AlfvenWave":
    dimensions = 2
    offset = [0.0, 0.0]
    size = [2 * pi, 2 * pi]
    end_time = 16.45
    periodic_bc = True
    function_name = "AlfvenWaveTest"

if args.end_time is not None:
    end_time = args.end_time
elif exahype2.Project._parse_project_arguments().build_mode == "Debug":
    end_time = 0.1

initial = (
    """
  GRMHD::InitialData::"""
    + function_name
    + """(
    x, 0.,
    Q
  );
"""
)

boundary_condition = (
    """
  GRMHD::InitialData::"""
    + function_name
    + """(
    x, t,
    Qoutside
  );
"""
)

analytical_solution = (
    """
  GRMHD::InitialData::"""
    + function_name
    + """(
    x, t,
    solution
  );
"""
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name=args.scenario,
    dimensions=dimensions,
    offset=(offset + [0.0, 0.0, 0.0])[:3],
    size=(size + [size[-1]])[:3],
    periodic_BC=[periodic_bc, periodic_bc, periodic_bc],
    executable="ExaHyPE",
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
    raise ValueError(
        f"Scenario {args.scenario} is {dimensions}D, but the project was configured "
        f"for {project.dimensions}D."
    )

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    time_step_relaxation=0.9,
    unknowns={
        "rho": 1,
        "vel": 3,
        "E": 1,
        "B": 3,
        "psi": 1,
        "lapse": 1,
        "shift": 3,
        "gij": 6,
    },
    auxiliary_variables=0,  # TODO: true?
)

solver.add_kernel_optimisations(
    polynomials=polynomials,
    is_linear=False,
    # precision=args.precision
)

solver.set_implementation(
    initial_conditions=initial,
    boundary_conditions=boundary_condition,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    ncp=ncp,
)

solver.add_user_solver_includes(
    """
#include "InitialData.h"
#include "PDE.h"
"""
)

exahype2.solvers.aderdg.ErrorMeasurement(
    solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error_"
    + args.scenario
    + "_md_"
    + str(args.grid_levels)
    + "_o_"
    + str(order),
)

project.add_solver(solver)

project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
