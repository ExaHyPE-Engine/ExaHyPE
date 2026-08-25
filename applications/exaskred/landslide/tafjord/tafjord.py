# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Tafjord 1934 rock slide descending dry terrain."""

import argparse
import math

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=80.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=8)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=3)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=3)
    parser.add_argument(
        "--friction",
        type=float,
        default=200.0,
        help="Voellmy-Salm turbulent friction length.",
    )
    parser.add_argument(
        "--water-friction-drag-coefficient",
        type=float,
        default=0.0,
        help="Kim et al. (2019) free-surface friction drag coefficient.",
    )
    parser.add_argument(
        "--water-pressure-drag-coefficient",
        type=float,
        default=0.0,
        help="Kim et al. (2019) pressure drag coefficient.",
    )
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
dg_order = max(1, args.degrees_of_freedom - 1)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="TafjordLandslide",
    dimensions=2,
    offset=[414910.5, 6904510.5],
    size=[1875.0, 1925.0],
    periodic_bc=[False, False],
    executable="ExaSkred-Landslide",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The Tafjord landslide application is two-dimensional.")

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": project.dimensions, "z": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.20,
)
aderdg_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
    max_eigenvalue=exahype2.solvers.PDETerms.User_Defined_Implementation,
    ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
    diffusive_source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
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
    number_of_dmp_observables=3,
    dmp_relaxation_parameter=0.0,
    dmp_differences_scaling=1.0e-3,
    dmp_observables=exahype2.solvers.PDETerms.User_Defined_Implementation,
    dmp_relaxation_parameter_implementation=exahype2.solvers.PDETerms.User_Defined_Implementation,
    physical_admissibility_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

project.add_solver(aderdg_solver)

for name, kind, value in (
    ("bathymetryFile", "string", "bathymetry/TafjordBathymetry.nc"),
    ("displacementFile", "string", "bathymetry/TafjordDisplacement.nc"),
    ("bathymetrySize", ("double", 2), [1900.0, 1950.0]),
    ("bathymetryOffset", ("double", 2), [414895.5, 6904495.5]),
    ("displacementSize", ("double", 2), [1900.0, 1950.0]),
    ("displacementOffset", ("double", 2), [414895.5, 6904495.5]),
    ("displacementPosition", ("double", 2), [0.0, 0.0]),
):
    project.add_runtime_parameter(name, kind, value)

project.constants.export_constexpr_with_type("g", str(9.81), "double")
# The slide descends dry terrain, so nothing submerges it: The water level and the
# water density are both zero, and the buoyancy of the slide is zero.
project.constants.export_constexpr_with_type("waterLevel", str(0.0), "double")
project.constants.export_constexpr_with_type("rhoSlide", str(1100.0), "double")
project.constants.export_constexpr_with_type("rhoWater", str(0.0), "double")
project.constants.export_constexpr_with_type("phi", str(25.0), "double")
project.constants.export_constexpr_with_type(
    "invXi", str(1.0 / args.friction), "double"
)
project.constants.export_constexpr_with_type(
    "waterFrictionDragCoefficient", str(args.water_friction_drag_coefficient), "double"
)
project.constants.export_constexpr_with_type(
    "waterPressureDragCoefficient", str(args.water_pressure_drag_coefficient), "double"
)
project.constants.export_constexpr_with_type("hThreshold", str(1.0e-1), "double")
project.constants.export_constexpr_with_type(
    "maxFrictionStiffnessRatio", str(2.0 * args.degrees_of_freedom), "double"
)
project.constants.export_constexpr_with_type(
    "maxHeightCurvatureRatio", str(0.3), "double"
)
project.constants.export_constexpr_with_type(
    "mu", str(math.tan(math.pi / 180.0 * 25.0)), "double"
)

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_cpp_file("../../AugmentedState.cpp")
project.output.makefile.add_cpp_file("../../Landslide.cpp")
project.output.makefile.add_cpp_file("../../LandslideLimiting.cpp")
project.output.makefile.add_cpp_file("../../WellBalancedRusanov.cpp")
project.output.makefile.add_cpp_file("ADERDGSolver.cpp")
project.generate_and_build()
