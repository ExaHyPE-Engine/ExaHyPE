# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

from PDE import max_eigenvalue, flux


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
    parser.add_argument("-l", "--grid-levels", type=int, default=6)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  Q.rho  = 0.02 * (1.0 + std::exp(-0.5 * tarch::la::norm2(x) * tarch::la::norm2(x) / 0.01));
  Q.rhoU = 1.0 * Q.rho;

  const tarch::la::Vector<DIMENSIONS, double> momenta(1.0 * Q.rho);

  // The energy keeps the pressure constant over the domain.
  Q.rhoE = PRESSURE / (GAMMA - 1.0) + 0.5 * (momenta * momenta) / Q.rho;
"""

analytical_solution = """
  // The initial condition moves unchanged with velocity 1 along every axis. The solution at x
  // is the initial value at x - t, wrapped into the periodic domain.
#if DIMENSIONS == 2
  tarch::la::Vector<DIMENSIONS, double> pos = { (x[0] - t) + (int)(t + .5 - x[0]),
                                                (x[1] - t) + (int)(t + .5 - x[1]) };
#else
  tarch::la::Vector<DIMENSIONS, double> pos = { (x[0] - t) + (int)(t + .5 - x[0]),
                                                (x[1] - t) + (int)(t + .5 - x[1]),
                                                (x[2] - t) + (int)(t + .5 - x[2]) };
#endif

  solution.rho  = 0.02 * (1.0 + std::exp(-0.5 * tarch::la::norm2(pos) * tarch::la::norm2(pos) / 0.01));
  solution.rhoU = 1.0 * solution.rho;

  const tarch::la::Vector<DIMENSIONS, double> momenta(1.0 * solution.rho);

  // The energy keeps the pressure constant over the domain.
  solution.rhoE = PRESSURE / (GAMMA - 1.0) + 0.5 * (momenta * momenta) / solution.rho;
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="GaussianBell",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[-0.5, -0.5, -0.5],
    periodic_bc=[True, True, True],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.fv.Solver(
    name="MUSCLHancockSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.MusclHancock(limiter=exahype2.fv.Limiter.minmod),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    analytical_solution=analytical_solution,
)

fv_solver.measure_error(file="Error", every=args.end_time / 100.0)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.output.makefile.add_compile_definition("PRESSURE=1.0")
project.generate_and_build()
