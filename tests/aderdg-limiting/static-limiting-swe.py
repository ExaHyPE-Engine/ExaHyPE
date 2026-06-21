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
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=1.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=6)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

dimensions = 2
order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="StaticLimitingSWE",
    dimensions=dimensions,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
    raise ValueError(
        f"Static limiting SWE is a {dimensions}D testcase, but the project was "
        f"configured for {project.dimensions}D."
    )

initial_conditions = """
  Q[0] = 4.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 2.0 - tarch::la::norm2(x);
"""

boundary_conditions = """
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
"""

flux = """
  double ih = 1.0 / Q[0];

  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;
"""

ncp = """
  constexpr double grav = 9.81;

  BTimesDeltaQ[0] = 0.0;
  switch (normal) {
  case 0:
    BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1:
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }
  BTimesDeltaQ[3] = 0.0;
"""

max_eval = """
  constexpr double grav = 9.81;

  double u = Q[1 + normal] / Q[0];
  double c = std::sqrt(grav * Q[0]);

  return std::max(std::abs(u + c), std::abs(u - c));
"""

limiting_criterion = """
bool isBoundaryCell = (
  x[0] + 1.00 <  0.5 * h[0] or
  x[1] + 1.00 <  0.5 * h[1] or
  x[0] - 1.00 > -0.5 * h[0] or
  x[1] - 1.00 > -0.5 * h[1]
);
return !isBoundaryCell;
"""

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    unknowns=4,
    auxiliary_variables=0,
    time_step_relaxation=0.9,
)
aderdg_solver.set_implementation(
    flux=flux,
    ncp=ncp,
    max_eigenvalue=max_eval,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
)
aderdg_solver.add_kernel_optimisations(
    polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Lobatto
)
project.add_solver(aderdg_solver)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=2 * order + 1,
    unknowns=4,
    auxiliary_variables=0,
    time_step_relaxation=0.9,
)
fv_solver.set_implementation(
    flux=flux,
    ncp=ncp,
    max_eigenvalue=max_eval,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
)
project.add_solver(fv_solver)

limiter = exahype2.solvers.limiting.StaticLimiting(
    name="LimitingSolver",
    regular_solver=aderdg_solver,
    limiting_solver=fv_solver,
    physical_admissibility_criterion=limiting_criterion,
)
project.add_solver(limiter)

project = project.generate_Peano4_project()
project.generate_and_build()
