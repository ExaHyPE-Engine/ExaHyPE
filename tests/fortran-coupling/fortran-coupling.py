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
    parser.add_argument("-et", "--end-time", type=float, default=0.2)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=11)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  const double r = std::sqrt(x(0) * x(0) + x(1) * x(1));
  Q[0] = r < 0.5 ? 2.0 : 1.0; // water depth h
  Q[1] = 0.0;                 // momentum h * u
  Q[2] = 0.0;                 // momentum h * v
  Q[3] = 0.0;                 // flat bathymetry b
"""

boundary_conditions = """
  Qoutside[0] = 1.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = 0.0;
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fortran"],
    project_name="FortranCoupling",
    dimensions=2,
    size=[4.0, 4.0, 4.0],
    offset=[-2.0, -2.0, -2.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 2, "b": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)
fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
    riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation,
)
project.add_solver(fv_solver)

project.output.makefile.add_cpp_file("FVSolver.cpp")
project.output.makefile.add_Fortran_file("rpn2_geoclaw.f90")

project.generate_and_build()
