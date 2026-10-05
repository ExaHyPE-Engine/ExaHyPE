# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
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
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=4)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=4)
    parser.add_argument(
        "--friction",
        type=float,
        default=200.0,
        help="Voellmy-Salm turbulent friction length.",
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
    project_name="CoupledTafjord",
    dimensions=2,
    offset=[414910.5, 6904510.5],
    size=[1875.0, 1925.0],
    periodic_bc=[False, False],
    executable="ExaSkred-Coupled",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The coupled Tafjord application is two-dimensional.")

aderdg_solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={
        "h": 1,
        "hu": project.dimensions,
        "lsh": 1,
        "lshu": project.dimensions,
        "z": 1,
    },
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.20),
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
    diffusive_source=True,
    riemann_solver=True,
)
aderdg_solver.set_pde_terms(
    subcell_riemann_solver=True,
    dmp_observables=True,
    dmp_relaxation_parameter=True,
    physical_admissibility=True,
)
aderdg_solver.add_subcell_limiter(
    exahype2.fv.Godunov(),
    number_of_dmp_observables=6,
    dmp_relaxation_parameter=0.0,
    dmp_differences_scaling=1.0e-3,
)

project.add_solver(aderdg_solver)

for name, kind, value in (
    (
        "bathymetryFile",
        "string",
        "../../landslide/tafjord/bathymetry/TafjordBathymetry.nc",
    ),
    (
        "displacementFile",
        "string",
        "../../landslide/tafjord/bathymetry/TafjordDisplacement.nc",
    ),
    ("bathymetrySize", ("double", 2), [1900.0, 1950.0]),
    ("bathymetryOffset", ("double", 2), [414895.5, 6904495.5]),
    ("displacementSize", ("double", 2), [1900.0, 1950.0]),
    ("displacementOffset", ("double", 2), [414895.5, 6904495.5]),
    ("displacementPosition", ("double", 2), [0.0, 0.0]),
):
    project.add_runtime_parameter(name, kind, value)

project.add_constant("g", 9.81)
project.add_constant("rhoSlide", 1100.0)
project.add_constant("rhoWater", 1025.0)
project.add_constant("phi", 25.0)
project.add_constant("invXi", 1.0 / args.friction)
project.add_constant("waterFrictionDragCoefficient", 0.01)
project.add_constant("waterPressureDragCoefficient", 1.0)
project.add_constant("hThresholdSlide", 3.0e-1)
project.add_constant("hThresholdWater", 0.5)
project.add_constant("waterDmpRelaxation", 0.1)
project.add_constant("maxFrictionStiffnessRatio", 2.0 * args.degrees_of_freedom)
project.add_constant("maxHeightCurvatureRatio", 0.3)
project.add_constant("mu", math.tan(math.pi / 180.0 * 25.0))

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_h_file("ADERDGSolver.cpph")
project.generate_and_build()
