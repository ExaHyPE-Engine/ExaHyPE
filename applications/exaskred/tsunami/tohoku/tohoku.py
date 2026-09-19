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

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=dg_order,
    unknowns={"h": 1, "hu": project.dimensions, "b": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.6,
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
aderdg_solver.add_subcell_limiter(
    limiter_solver=exahype2.solvers.fv.godunov.subcell_scheme(
        riemann_solver=exahype2.solvers.PDETerms.User_Defined_Implementation
    ),
    number_of_dmp_observables=4,
    dmp_relaxation_parameter=10000.0,
    dmp_differences_scaling=10000.0,
    dmp_observables=exahype2.solvers.PDETerms.User_Defined_Implementation,
    physical_admissibility_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
)

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

    project.add_action_set_to_initialisation(
        exahype2.tracer.InsertParticlesByCoordinates(
            particle_set=probe_particles,
            coordinates=[
                [545735.266126, 62716.4740303],
                [1050678.21, 798352.124],
            ],
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DiscontinuousGalerkinTracing(
            probe_particles,
            aderdg_solver,
            project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
        )
    )

    project.add_action_set_to_timestepping(
        exahype2.tracer.DumpTracerIntoDatabase(
            particle_set=probe_particles,
            solver=aderdg_solver,
            filename="Probes",
            time_delta_between_two_snapshots=10.0,
            output_precision=17,
        )
    )

project.constants.export_constexpr_with_type("g", str(9.81), "double")
project.constants.export_constexpr_with_type("hThreshold", str(1.0), "double")
project.constants.export_constexpr_with_type(
    "limiterBathymetryThreshold", str(10.0), "double"
)
project.constants.export_constexpr_with_type(
    "limiterRelativeFreeSurfaceVariation", str(3.0e-4), "double"
)
project.constants.export_constexpr_with_type(
    "limiterRelativeNegativeFreeSurfaceVariation", str(1.6e-4), "double"
)
project.constants.export_constexpr_with_type(
    "limiterRelativeSourceFreeSurfaceAmplitude", str(1.0e-3), "double"
)
project.constants.export_constexpr_with_type(
    "sourceLowPassEffectiveRadius", str(12.0 * displacement_grid_spacing), "double"
)
project.constants.export_constexpr_with_type(
    "sourceFilterMinimumCellH",
    str(min([1798000.0, 1798000.0]) / (3.0**args.grid_levels)),
    "double",
)

project.output.makefile.add_include_directory("../../../..")
project.output.makefile.add_cpp_file("ADERDGSolver.cpp")
project.generate_and_build()
