###
### A sample Python file for this problem:
###
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
    parser.add_argument("-et", "--end-time", type=float, default=10.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=22)
    parser.add_argument("-l", "--grid-levels", type=int, default=5)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

domain_size = [120.0, 120.0]
domain_offset = [-10.0, -60.0]

unknowns = {"p": 1, "v": 2, "E": 1}
auxiliary_variables = {}

exahype2_project = exahype2.Project(
    namespace=["tutorials", "exahype2", "euler"],
    project_name="Airfoil",
    dimensions=2,
    size=domain_size + [domain_size[-1]],
    offset=domain_offset + [0.0],
    periodic_bc=[False, False, False],
    executable="Airfoil",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if exahype2_project.dimensions != 2:
    raise ValueError(
        f"Euler tutorial is a 2D testcase, but the project was configured for "
        f"{exahype2_project.dimensions}D."
    )

my_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="EulerSolver",
    patch_size=args.degrees_of_freedom,
    time_step_relaxation=0.5,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
)

my_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

exahype2_project.add_solver(my_solver)

exahype2_project.output.makefile.add_cpp_file("EulerSolver.cpp")
exahype2_project.generate_and_build()
