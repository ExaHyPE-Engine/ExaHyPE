# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import math

import exahype2


def manning_coefficient(depth, roughness_length=0.003):
    """Returns the Manning coefficient of a channel of the given depth, after Chezy."""
    if depth <= 0.0:
        raise ValueError("Water depth must be positive.")
    return depth ** (1.0 / 6.0) / (18.0 * math.log10(0.37 * depth / roughness_length))


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=100.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=7)
    parser.add_argument("-l", "--grid-levels", type=int, default=2)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

channel_offset = [-39870.0, 0.0]
channel_size = [379700.0, 100e3]
mesh_side = max(channel_size)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="MeteotsunamiR1",
    dimensions=2,
    offset=[
        channel_offset[0] - 0.5 * (mesh_side - channel_size[0]),
        channel_offset[1] - 0.5 * (mesh_side - channel_size[1]),
    ],
    size=[mesh_side, mesh_side],
    periodic_bc=[False, False],
    executable="ExaSkred-Meteotsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The R1 meteotsunami application is two-dimensional.")

aderdg_solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=max(1, args.degrees_of_freedom - 1),
    unknowns={"h": 1, "hu": project.dimensions, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.6),
)
aderdg_solver.set_pde_terms(
    initial_condition=True,
    boundary_conditions=True,
    refinement_criterion=True,
    flux=True,
    max_eigenvalue=True,
    ncp=True,
    source=True,
    riemann_solver=True,
)
aderdg_solver.set_pde_terms(
    subcell_riemann_solver=True,
    dmp_observables=True,
    physical_admissibility=True,
)
aderdg_solver.add_subcell_limiter(
    exahype2.fv.Godunov(),
    number_of_dmp_observables=4,
    dmp_relaxation_parameter=1.0e-2,
    dmp_differences_scaling=1.0e-3,
)
project.add_solver(aderdg_solver)

for name, kind, value in (
    ("bathymetryFile", "string", "bathymetry/R1BathymetryDepth50m.nc"),
    ("bathymetryOffset", ("double", 2), channel_offset),
    ("bathymetrySize", ("double", 2), channel_size),
    ("minimumWaterHeight", "double", 1.0),
    ("coastalDepth", "double", 5.0),
    ("waterDensity", "double", 1000.0),
    ("manningCoefficient", "double", manning_coefficient(50.0)),
    ("coriolisParameter", "double", 9.35e-5),
    ("pressureAmplitude", "double", 100.0),
    ("pressureSpeed", "double", 22.15),
    ("pressurePeriod", "double", 1800.0),
):
    project.add_runtime_parameter(name, kind, value)

project.add_constant("g", 9.81)

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_h_file("ADERDGSolver.cpph")
project.generate_and_build()
