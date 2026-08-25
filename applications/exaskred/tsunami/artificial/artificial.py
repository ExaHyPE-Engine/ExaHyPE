# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Artificial tsunami over a synthetic seamount and its uplift."""

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

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": project.dimensions, "b": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)

aderdg_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

aderdg_solver.add_kernel_optimisations(
    is_linear=False,
    polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
)
aderdg_solver.add_subcell_limiter(
    limiter_solver=exahype2.solvers.fv.godunov.subcell_scheme(
        riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation
    ),
    physical_admissibility_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

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

project.constants.export_constexpr_with_type("g", str(9.81), "double")
project.constants.export_constexpr_with_type("hThreshold", str(1e-5), "double")

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_cpp_file("../../AugmentedState.cpp")
project.output.makefile.add_cpp_file("../../Limiting.cpp")
project.output.makefile.add_cpp_file("../../ShallowWater.cpp")
project.output.makefile.add_cpp_file("../../WellBalancedRusanov.cpp")
project.output.makefile.add_cpp_file("ADERDGSolver.cpp")
project.generate_and_build()
