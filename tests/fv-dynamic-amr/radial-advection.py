# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.4)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=2)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=4)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="RadialAdvection",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

initial_conditions = """
  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  constexpr double Radius = 0.15;
  Q[Shortcuts::rho] = tarch::la::smallerEquals(tarch::la::norm2(x - centre), Radius) ? 1.0 : 0.0;
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

flux = """
  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> radialOffset = x - centre;
  const double radius = tarch::la::norm2(radialOffset);
  F[Shortcuts::rho] = (radius > 0.0 ? radialOffset(normal) / radius : 0.0) * Q[Shortcuts::rho];
"""

# The unit offset keeps relative detail bounded where density is zero.
refinement_criterion = """
  return ::exahype2::refineOnDetail(
    Q,
    NumberOfFiniteVolumesPerAxisPerPatch,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1.0e-2,
    9.0,
    nullptr,
    [](const double* const values) { return 1.0 + values[Shortcuts::rho]; }
  );
"""

solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.8,
)

solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue="return 1.0;",
)

project.add_solver(solver)

project.generate_and_build()
