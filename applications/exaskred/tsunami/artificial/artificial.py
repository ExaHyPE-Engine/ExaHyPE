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
    parser.add_argument("-et", "--end-time", type=float, default=50.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=4)
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
dg_order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="ArtificialTsunami",
    dimensions=2,
    offset=[0.0, 0.0],
    size=[10000.0, 10000.0],
    periodic_bc=[False, False],
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The artificial tsunami application is two-dimensional.")

aderdg_solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": project.dimensions, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    basis=exahype2.dg.GaussLegendre,
    linear=False,
)

aderdg_solver.set_pde_terms(
    initial_condition=True,
    boundary_conditions=True,
    refinement_criterion=True,
    flux=True,
    max_eigenvalue=True,
    ncp=True,
    riemann_solver=True,
)

aderdg_solver.set_pde_terms(
    subcell_riemann_solver=True,
    physical_admissibility=True,
)
aderdg_solver.add_subcell_limiter(exahype2.fv.Godunov())

project.add_solver(aderdg_solver)

for name, kind, value in (
    ("bathymetryFile", "string", "bathymetry/ArtificialBathymetry.nc"),
    ("displacementFile", "string", "bathymetry/ArtificialDisplacement.nc"),
    ("bathymetrySize", ("double", 2), [10000.0, 10000.0]),
    ("bathymetryOffset", ("double", 2), [0.0, 0.0]),
    ("displacementSize", ("double", 2), [10000.0, 10000.0]),
    ("displacementOffset", ("double", 2), [0.0, 0.0]),
    ("displacementPosition", ("double", 2), [0.0, 0.0]),
):
    project.add_runtime_parameter(name, kind, value)

project.add_constant("g", 9.81)
project.add_constant("hThreshold", 1e-5)

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_cpp_file("ADERDGSolver.cpp")
project.generate_and_build()
