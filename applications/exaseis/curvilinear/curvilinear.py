import argparse
import os
import sys

import exahype2

import CurvilinearElastic

sys.path.insert(0, os.path.abspath(".."))
import Scenario

available_scenarios = {"LOH1": Scenario.LOH1(), "Zugspitze": Scenario.Zugspitze()}


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument(
        "-s", "--scenario", choices=available_scenarios.keys(), default="LOH1"
    )
    parser.add_argument("-et", "--end-time", type=float, default=None)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-p", "--precision", type=str, default="fp64")
    parser.add_argument(
        "-ut", "--use-tracers", action=argparse.BooleanOptionalAction, default=True
    )
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
my_scenario = available_scenarios[args.scenario]
order = args.degrees_of_freedom
end_time = my_scenario.end_time if args.end_time is None else args.end_time

my_scenario.generate_required_files()

project = exahype2.Project(
    ["applications", "exahype2", "exaseis"],
    "curvi",
    dimensions=3,
    offset=my_scenario.domain_offset,
    size=my_scenario.domain_size,
    periodic_bc=[False, False, False],
    executable="ExaSeis",
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 3:
    raise ValueError("The curvilinear ExaSeis application is three-dimensional.")

theSolver = exahype2.dg.ader.Solver(
    name="ElasticSolver",
    order=order,
    unknowns={"v": 3, "sigma": 6},
    auxiliary_variables={
        "rho": 1,
        "cp": 1,
        "cs": 1,
        "jacobian": 1,
        "metricDerivative": 9,
        "curveGrid": 3,
    },
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=exahype2.dg.GaussLobatto,
    linear=True,
    precision=args.precision,
    point_sources=1,
    initialise_patches=True,
)

theSolver.set_pde_terms(
    initial_condition=CurvilinearElastic.initial(my_scenario.initial_conditions()),
    boundary_conditions=CurvilinearElastic.boundary(),
    max_eigenvalue=CurvilinearElastic.eigenvalue(),
    refinement_criterion=CurvilinearElastic.refinement_criterion(),
    flux=CurvilinearElastic.flux(),
    ncp=CurvilinearElastic.ncp(),
    material_parameters=CurvilinearElastic.multiplyMaterialParameterMatrix(),
    riemann_solver=CurvilinearElastic.riemann_solver(),
    point_source=my_scenario.point_source(),
    point_source_locations=my_scenario.init_point_source(),
)

theSolver.add_declarations(CurvilinearElastic.abstractDeclarations())
theSolver.add_definitions(CurvilinearElastic.abstractUserDefinitions())
theSolver.on_start_grid_initialisation(
    CurvilinearElastic.init_grid_step_implementation(my_scenario.domain_perturbations())
)
theSolver.add_includes(*CurvilinearElastic.userIncludes())
# The initial condition reads the curvilinear mapping and the NetCDF samplers of the host.
theSolver.initialise_on_host()

project.add_solver(theSolver)
project.output.makefile.add_include_directory("../../..")

if args.use_tracers:
    # The tracers sit at the mesh positions of the physical coordinates and record every unknown.
    tracer_particles = project.add_tracer(
        name="Tracer", particle_attributes=theSolver.state.unknowns
    )
    tracer_particles.insert_by_coordinates(
        my_scenario.tracer_coordinates, transform="invertProjection"
    )
    tracer_particles.dump_into_database(
        filename=args.scenario + "-o" + str(order) + "-" + str(args.precision),
        time_delta_between_two_snapshots=0.01,
        output_precision=10,
    )
    theSolver.trace(
        tracer_particles,
        exahype2.particles.Forces(
            interpolation_scheme=2,
            integration_scheme=0,
            force_method=0,
            fields_to_interpolate=",".join(
                str(field) for field in range(theSolver.state.unknowns)
            ),
        ),
        initialisation=False,
    )

project.generate_and_build()
