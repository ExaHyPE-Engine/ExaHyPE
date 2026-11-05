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
  Q.h  = r < 0.5 ? 2.0 : 1.0;
  Q.hu = 0.0;
  Q.b  = 0.0;
"""

boundary_conditions = """
  Qoutside.h  = 1.0;
  Qoutside.hu = 0.0;
  Qoutside.b  = 0.0;
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

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 2, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.Godunov(),
)
fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=True,
    riemann_solver=True,
)
project.add_solver(fv_solver)

project.output.makefile.add_cpp_file("FVSolver.cpp")
project.output.makefile.add_fortran_file("rpn2_geoclaw.f90")

project.generate_and_build()
