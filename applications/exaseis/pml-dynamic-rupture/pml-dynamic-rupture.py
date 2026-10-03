# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import os
import sys

import exahype2

sys.path.insert(0, os.path.abspath("../curvilinear"))
sys.path.insert(0, os.path.abspath("../pml"))
sys.path.insert(0, os.path.abspath("../dynamic-rupture"))

import CurvilinearElastic
import PMLElastic
import DynamicRuptureElastic
import PMLDynamicRuptureElastic

sys.path.insert(0, os.path.abspath(".."))
import Scenario

available_scenarios = {
    "TPV5": Scenario.TPV5(),
    "TPV6": Scenario.TPV6(),
    "TPV16": Scenario.TPV16(),
    "TPV26": Scenario.TPV26(),
    "TPV28": Scenario.TPV28(),
    "TPV29": Scenario.TPV29(),
    "TPV34": Scenario.TPV34(),
    "Husavik": Scenario.Husavik(),
}


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument(
        "-s", "--scenario", choices=available_scenarios.keys(), default="TPV5"
    )
    parser.add_argument("-et", "--end-time", type=float, default=None)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    parser.add_argument("-p", "--precision", type=str, default="fp64")
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
    "pml",
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
    raise ValueError(
        "The PML dynamic rupture ExaSeis application is three-dimensional."
    )

theSolver = exahype2.dg.ader.Solver(
    name="ElasticSolver",
    order=order,
    unknowns={"v": 3, "sigma": 6, "u": 3, "pml": 27},
    auxiliary_variables={
        "rho": 1,
        "cp": 1,
        "cs": 1,
        "pmlDamping": 3,
        "jacobian": 1,
        "metricDerivative": 9,
        "curveGrid": 3,
    },
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    basis=exahype2.dg.GaussLobatto,
    linear=True,
    precision=args.precision,
    initialise_patches=True,
)

theSolver.set_pde_terms(
    initial_condition=CurvilinearElastic.initial(my_scenario.initial_conditions())
    + PMLElastic.initial(),
    boundary_conditions=CurvilinearElastic.boundary(),
    max_eigenvalue=CurvilinearElastic.eigenvalue(),
    refinement_criterion=CurvilinearElastic.refinement_criterion(),
    flux=PMLElastic.flux(),
    ncp=PMLElastic.ncp(),
    material_parameters=PMLElastic.multiplyMaterialParameterMatrix(),
    riemann_solver=DynamicRuptureElastic.riemann_solver(),
    source=PMLElastic.algebraic_source() + PMLDynamicRuptureElastic.algebraic_source(),
)

theSolver.add_declarations(
    CurvilinearElastic.abstractDeclarations() + PMLElastic.abstractDeclarations()
)
theSolver.add_definitions(CurvilinearElastic.abstractUserDefinitions())
theSolver.on_start_grid_initialisation(
    DynamicRuptureElastic.init_grid_step_implementation(
        my_scenario.domain_perturbations()
    )
)
theSolver.add_includes(*PMLDynamicRuptureElastic.userIncludes())
# The initial condition reads the curvilinear mapping and the NetCDF samplers of the host.
theSolver.initialise_on_host()
project.add_constant("PmlTop", 1, "int")

project.add_solver(theSolver)
project.output.makefile.add_include_directory("../../..")
project.output.makefile.add_h_file("../DynamicRuptureParameters.h", generated=True)
project.output.makefile.add_h_file("../DynamicRuptureParameters.cpph", generated=True)

filename = args.scenario + "-o" + str(order) + "-" + str(args.precision)
recorded_unknowns = 12
recorded_fields = ",".join(str(field) for field in range(recorded_unknowns))

# The tracers record the velocity, the stress and the displacement, and skip the PML unknowns.
tracer_particles = project.add_tracer(
    name="Tracer", particle_attributes=recorded_unknowns
)
tracer_particles.insert_by_coordinates(
    my_scenario.tracer_coordinates, transform="invertProjection"
)
tracer_particles.dump_into_database(
    filename=filename, time_delta_between_two_snapshots=0.01, output_precision=10
)
theSolver.trace(
    tracer_particles,
    exahype2.particles.Forces(
        interpolation_scheme=2,
        integration_scheme=0,
        force_method=0,
        fields_to_interpolate=recorded_fields,
    ),
    initialisation=False,
)

# The jumps of u and v across the fault are the slip and the slip rate.
if my_scenario.fault_coordinates:
    fault_particles = project.add_tracer(
        name="FaultTracer", particle_attributes=recorded_unknowns
    )
    fault_particles.insert_by_coordinates(
        my_scenario.fault_coordinates, transform="invertProjection"
    )
    fault_particles.sample_face_traces(
        normal=DynamicRuptureElastic.fault_normal(),
        position=DynamicRuptureElastic.fault_position(),
        difference_fields=["u", "v"],
    )
    fault_particles.dump_into_database(
        filename=filename + "-fault",
        time_delta_between_two_snapshots=0.01,
        output_precision=10,
    )
    theSolver.trace(fault_particles, initialisation=False)

theSolver.plot_faces(DynamicRuptureElastic.fault_face_predicate())

project.generate_and_build()
