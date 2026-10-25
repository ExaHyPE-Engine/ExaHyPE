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
#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> circleCentre = {0.0, -0.2};
#else
  const tarch::la::Vector<DIMENSIONS, double> circleCentre = {-0.32, -0.2, 0.1};
#endif

  // The standard deviation of the Gaussian is the largest admissible patch size. The coarsest grid
  // then resolves the Gaussian smoothly.
  const double peakDeviation = RuntimeParameters::getMaxAdmissiblePatchSize();
  const double distance = tarch::la::norm2(x - circleCentre);
  const double exponent = -(distance * distance) / 2.0 / peakDeviation / peakDeviation;

  Q[Shortcuts::rho] = 0.1;
  Q[Shortcuts::rhoU + 0] = 0.0;
  Q[Shortcuts::rhoU + 1] = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2] = 0.0;
#endif
  Q[Shortcuts::rhoE] = 1.0 + std::exp(exponent);
"""

boundary_conditions = """
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoU + 2] = -Qinside[Shortcuts::rhoU + 2];
#endif
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="GaussianExplosion",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[-0.5, -0.5, -0.5],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.Godunov(),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.generate_and_build()
