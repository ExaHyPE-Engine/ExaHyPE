# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse

import exahype2


def parse_arguments():
    available_precisions = ["float", "double"]
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument(
        "-pr", "--precision", choices=available_precisions, default="double"
    )
    parser.add_argument("-pbc", "--periodic-bc", choices=["on", "off"], default="on")
    parser.add_argument("-et", "--end-time", type=float, default=0.1)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=3)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=6)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

periodic = args.periodic_bc == "on"

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="EulerCircularDisc",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[periodic, periodic, periodic],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

initial_conditions = """
  Q[Shortcuts::rho]       = 1.0;
  Q[Shortcuts::rhoU + 0]  = 0.0;
  Q[Shortcuts::rhoU + 1]  = 0.0;
#if DIMENSIONS == 2
  Q[Shortcuts::rhoE]      = ((std::sqrt(std::pow(x(0), 2) + std::pow(x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[Shortcuts::rhoU + 2]  = 0.0;
  Q[Shortcuts::rhoE]      = ((std::sqrt(std::pow(x(0), 2) + std::pow(x(1), 2) + std::pow(x(2), 2)) < 0.2) ? (1.0) : (1.01));
#endif
"""

boundary_conditions = """
  // The solver calls these reflecting walls only with --periodic-bc off.
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoU + 2] = -Qinside[Shortcuts::rhoU + 2];
#endif
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

  // The refined disc lies at the domain corner. With periodic boundaries the refinement also covers
  // the far side of every axis. With reflecting walls the refinement stays in the corner as the
  // control case.
  tarch::la::Vector<DIMENSIONS, double> circleCentre(0.0);

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - circleCentre) < 0.1) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
"""

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q[Shortcuts::rho];
  const auto u0 = Q[Shortcuts::rhoU + 0] * irho;
  const auto u1 = Q[Shortcuts::rhoU + 1] * irho;
#if DIMENSIONS == 3
  const auto u2 = Q[Shortcuts::rhoU + 2] * irho;
#endif

#if DIMENSIONS == 3
  const auto uSq = tarch::la::sumOfSquaresInOrderOfMagnitude(u0, u1, u2);
  const auto u_n = (normal == 0) ? u0 : (normal == 1) ? u1 : u2;
#else
  const auto squareOfU0 = tarch::la::productRoundedOnce(u0, u0);
  const auto squareOfU1 = tarch::la::productRoundedOnce(u1, u1);
  const auto uSq        = squareOfU0 + squareOfU1;
  const auto u_n = (normal == 0) ? u0 : u1;
#endif

  const auto internalE = Q[Shortcuts::rhoE] - 0.5 * Q[Shortcuts::rho] * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  auto result = std::fmax(0.0, std::fabs(u_n - speedOfSound));
  result = std::fmax(result, std::fabs(u_n + speedOfSound));
  return result;
""".format(compute_primitive_variables=compute_primitive_variables)

flux = r"""
  {compute_primitive_variables}

  F[Shortcuts::rho] = Q[Shortcuts::rhoU + normal];

  F[Shortcuts::rhoU + 0] = Q[Shortcuts::rhoU + 0] * u_n;
  F[Shortcuts::rhoU + 1] = Q[Shortcuts::rhoU + 1] * u_n;
#if DIMENSIONS == 3
  F[Shortcuts::rhoU + 2] = Q[Shortcuts::rhoU + 2] * u_n;
#endif

  F[Shortcuts::rhoU + normal] += p;

  F[Shortcuts::rhoE] = (Q[Shortcuts::rhoE] + p) * u_n;
""".format(compute_primitive_variables=compute_primitive_variables)

fv_solver = exahype2.fv.singlestep.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.Godunov(),
    precision=args.precision,
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.generate_and_build()
