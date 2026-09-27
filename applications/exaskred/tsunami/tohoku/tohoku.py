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
    parser.add_argument("-et", "--end-time", type=float, default=6820.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=3)
    parser.add_argument("-l", "--grid-levels", type=int, default=5)
    parser.add_argument(
        "--use-probes",
        action="store_true",
        help="Insert stationary probes and dump tracer data at their positions.",
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
dg_order = max(1, args.degrees_of_freedom - 1)

displacement_grid_spacing = min(
    498000.0 / (250 - 1),
    798000.0 / (400 - 1),
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="TohokuTsunami",
    dimensions=2,
    offset=[-499000.0, -949000.0],
    size=[1798000.0, 1798000.0],
    periodic_bc=[False, False],
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The Tohoku tsunami application is two-dimensional.")

aderdg_solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=dg_order,
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
    dmp_relaxation_parameter=10000.0,
    dmp_differences_scaling=10000.0,
)
aderdg_solver.initialise_on_host()

project.add_solver(aderdg_solver)

for name, kind, value in (
    ("bathymetryFile", "string", "bathymetry/TohokuBathymetry.nc"),
    ("displacementFile", "string", "bathymetry/TohokuDisplacement.nc"),
    ("bathymetrySize", ("double", 2), [6998000.0, 3998000.0]),
    ("bathymetryOffset", ("double", 2), [-499000.0, -2490000.0]),
    ("displacementSize", ("double", 2), [498000.0, 798000.0]),
    ("displacementOffset", ("double", 2), [-249000.0, -399000.0]),
    ("displacementPosition", ("double", 2), [0.0, 0.0]),
):
    project.add_runtime_parameter(name, kind, value)

if args.use_probes:
    probe_particles = project.add_tracer(name="Probe", particle_attributes=4)
    probe_particles.insert_by_coordinates(
        [
            [545735.266126, 62716.4740303],
            [1050678.21, 798352.124],
        ]
    )
    probe_particles.dump_into_database(
        filename="Probes", time_delta_between_two_snapshots=10.0, output_precision=17
    )
    aderdg_solver.trace(
        probe_particles,
        exahype2.particles.Forces(
            interpolation_scheme=2,
            integration_scheme=0,
            force_method=0,
            fields_to_interpolate="0,1,2,3",
        ),
        initialisation=False,
    )

project.add_constant("g", 9.81)
project.add_constant("hThreshold", 1.0)
project.add_constant("limiterBathymetryThreshold", 10.0)
project.add_constant("limiterRelativeFreeSurfaceVariation", 3.0e-4)
project.add_constant("limiterRelativeNegativeFreeSurfaceVariation", 1.6e-4)
project.add_constant("limiterRelativeSourceFreeSurfaceAmplitude", 1.0e-3)
project.add_constant("sourceLowPassEffectiveRadius", 12.0 * displacement_grid_spacing)
project.add_constant(
    "sourceFilterMinimumCellH", min([1798000.0, 1798000.0]) / (3.0**args.grid_levels)
)

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_h_file("ADERDGSolver.cpph")
project.generate_and_build()
