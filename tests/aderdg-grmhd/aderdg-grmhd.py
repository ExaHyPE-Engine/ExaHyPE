# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

from numpy import pi

from GRMHD import max_eigenvalue, flux, ncp

available_scenarios = ["AlfvenWave", "MichelSphericalAccretion"]


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument("-et", "--end-time", type=float, default=None)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=6)
    parser.add_argument("-l", "--grid-levels", type=int, default=1)
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

if args.scenario == "MichelSphericalAccretion":
    dimensions = 3
    offset = [2.2, 2.2, 2.2]
    size = [6.0, 6.0, 6.0]
    end_time = 0.02
    periodic_bc = False
    function_name = "setMichelAccretionState"
elif args.scenario == "AlfvenWave":
    dimensions = 2
    offset = [0.0, 0.0]
    size = [2 * pi, 2 * pi]
    end_time = 0.1
    periodic_bc = True
    function_name = "setAlfvenWaveState"

if args.end_time is not None:
    end_time = args.end_time

initial = (
    """
  tests::exahype2::aderdg::grmhd::"""
    + function_name
    + """(
    x, 0.,
    Q
  );
"""
)

boundary_condition = (
    """
  tests::exahype2::aderdg::grmhd::"""
    + function_name
    + """(
    x, t,
    Qoutside
  );
"""
)

analytical_solution = (
    """
  tests::exahype2::aderdg::grmhd::"""
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
    periodic_bc=[periodic_bc, periodic_bc, periodic_bc],
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
    raise ValueError(f"The scenario {args.scenario} runs in {dimensions}D only.")

solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=order,
    unknowns={
        "rho": 1,
        "vel": 3,
        "E": 1,
        "B": 3,
        "psi": 1,
        "lapse": 1,
        "shift": 3,
        "gij": exahype2.SymmetricMatrix(3),
    },
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=exahype2.dg.GaussLegendre,
    linear=False,
)

solver.set_pde_terms(
    initial_condition=initial,
    boundary_conditions=boundary_condition,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    ncp=ncp,
    analytical_solution=analytical_solution,
)

solver.add_includes("InitialData.h", "PDE.h")

solver.measure_error(
    file="Error_" + args.scenario + "_md_" + str(args.grid_levels) + "_o_" + str(order),
    every=end_time / 100.0,
)

project.add_solver(solver)

project.generate_and_build()
