import argparse
import os
import sys

import exahype2
from exahype2.solvers.aderdg.ADERDG import Polynomials

import PMLElastic

sys.path.insert(0, os.path.abspath("../ExaSeis_core"))
import Scenario

available_scenarios = {
    "LOH1": Scenario.LOH1(),
    "Zugspitze": Scenario.Zugspitze()
}

scenario = "LOH1"

scenario_string = scenario.lower()
my_scenario = available_scenarios[scenario]

unknowns = {"v": 3, "sigma": 6, "pml": 27}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "dmp_pml": 3, "jacobian": 1,
    "metric_derivative": 9,
    "curve_grid": 3,
}

offset  = my_scenario.domain_offset
size    = my_scenario.domain_size

order = 5

min_level = 3
max_depth = 0

precision = "fp64"
use_tracers = True

def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=my_scenario.end_time)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=order)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=min_level)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=min_level + max_depth)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
order = args.degrees_of_freedom
end_time = args.end_time

my_scenario.generate_required_files(order)

project = exahype2.Project(
    ["applications", "exahype2", "exaseis"],
    "pml",
    dimensions=3,
    offset=offset,
    size=size,
    periodic_bc=[False, False, False],
    executable=scenario+"_l_"+str(args.min_grid_levels)+"_o_"+str(order)+"_pr_"+precision,
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 3:
    raise ValueError("The PML ExaSeis application is three-dimensional.")

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    time_step_relaxation=0.9
)

theSolver.add_kernel_optimisations(
    is_linear=True,
    polynomials=Polynomials.Gauss_Lobatto,
    initialise_patches=True,
    precision = precision
)

theSolver.set_implementation(
    initial_conditions    = PMLElastic.initial(my_scenario.initial_conditions()),
    boundary_conditions   = PMLElastic.boundary(),
    max_eigenvalue        = PMLElastic.eigenvalue(),
    refinement_criterion  = PMLElastic.refinement_criterion(),
    flux                  = PMLElastic.flux(),
    ncp                   = PMLElastic.ncp(),
    material_parameters   = PMLElastic.multiplyMaterialParameterMatrix(),
    riemann_solver        = PMLElastic.riemann_solver(),
    source_term           = PMLElastic.algebraic_source(),
    number_of_point_sources = 1,
    point_source = my_scenario.point_source(),
    init_point_source_location = my_scenario.init_point_source()
)

theSolver._abstract_solver_user_definitions  += PMLElastic.abstractUserDefinitions()
theSolver._abstract_solver_user_declarations += PMLElastic.abstractDeclarations()
theSolver._start_grid_initialisation_implementation +=  PMLElastic.init_grid_step_implementation(scenario_string)
theSolver._destructor_implementation +=  PMLElastic.abstractDestructor()
theSolver.add_user_solver_includes(PMLElastic.userIncludes())

project.add_solver(theSolver)

if use_tracers:
    project.add_action_set_to_timestepping(
    exahype2.tracer.CurviTracer(
        coordinates=my_scenario.tracer_coordinates,
        solver=theSolver,
        filename=scenario+"_level_"+str(args.min_grid_levels)+"-order-"+str(order)+"-precision-"+str(precision),
        time_delta_between_two_snapshots=0.01,
        output_precision=10
    ))

project.generate_and_build()
