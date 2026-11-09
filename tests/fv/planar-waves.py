# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

max_eigenvalue = """
  return std::abs(Q[Shortcuts::cp]);
"""

flux = """
    // Lamé parameters
    const auto iRho   = 1/Q[Shortcuts::rho];
    const auto Mu     = Q[Shortcuts::rho] * Q[Shortcuts::cs] * Q[Shortcuts::cs];          //rho*cs*cs
    const auto Lambda = Q[Shortcuts::rho] * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * Mu; //rho*cp*cp - 2 Mu

    switch (normal) {
    case 0:
      F[Shortcuts::v+0] = -iRho * Q[Shortcuts::sigma+0];
      F[Shortcuts::v+1] = -iRho * Q[Shortcuts::sigma+2];
      F[Shortcuts::sigma+0] = -(2 * Mu + Lambda) * Q[Shortcuts::v+0];
      F[Shortcuts::sigma+1] = -Lambda * Q[Shortcuts::v+0];
      F[Shortcuts::sigma+2] = -Mu * Q[Shortcuts::v+1];
      break;
    case 1:
      F[Shortcuts::v+0] = -iRho * Q[Shortcuts::sigma+2];
      F[Shortcuts::v+1] = -iRho * Q[Shortcuts::sigma+1];
      F[Shortcuts::sigma+0] = -Lambda * Q[Shortcuts::v+1];
      F[Shortcuts::sigma+1] = -(2 * Mu + Lambda) * Q[Shortcuts::v+1];
      F[Shortcuts::sigma+2] = -Mu * Q[Shortcuts::v+0];
      break;
    }
"""

def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=4)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  // Lamé parameters
  constexpr auto mu     = 1;
  constexpr auto lambda = 2;

  // Constant auxiliary parameters
  constexpr auto rho  = 1;
  constexpr auto cp   = 2;
  constexpr auto cs   = 1;

  // Auxiliary variables
  Q[Shortcuts::rho] = rho;  //rho
  Q[Shortcuts::cp]  = cp;   //cp
  Q[Shortcuts::cs]  = cs;   //cs

  const double val_x = cos( -std::numbers::pi*x[0]);
  const double val_y = cos( -std::numbers::pi*x[1]);

  // Stresses
  Q[Shortcuts::sigma+0] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  Q[Shortcuts::sigma+1] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  Q[Shortcuts::sigma+2] = 0; //xy

  // Velocities
  Q[Shortcuts::v+0] = val_x*cp; //vx
  Q[Shortcuts::v+1] = val_y*cp; //vy
"""

analytical_solution = """
  // Lamé parameters
  constexpr auto mu     = 1;
  constexpr auto lambda = 2;

  // Constant auxiliary parameters
  constexpr auto cp   = 2;

  constexpr double w = -2*M_PI;

  const double val_x = cos( w*t -std::numbers::pi*x[0]);
  const double val_y = cos( w*t -std::numbers::pi*x[1]);

  // Stresses
  solution[Shortcuts::sigma+0] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  solution[Shortcuts::sigma+1] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  solution[Shortcuts::sigma+2] = 0; //xy

  // Velocities
  solution[Shortcuts::v+0] = val_x*cp; //vx
  solution[Shortcuts::v+1] = val_y*cp; //vy
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="PlanarWave",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_bc=[True, True, True],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"v": 2, "sigma": 3},
    auxiliary_variables={"rho": 1, "cp": 1, "cs": 1},
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.Godunov(),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    analytical_solution=analytical_solution
)

project.add_solver(fv_solver)

project.generate_and_build()
