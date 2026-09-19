import argparse
import os, sys

import exahype2

sys.path.insert(0, os.path.abspath("../curvilinear"))

import CurvilinearElastic
import PMLElastic

sys.path.insert(0, os.path.abspath(".."))
import Scenario

available_scenarios = {
    "LOH1": Scenario.LOH1(),
    "Zugspitze": Scenario.Zugspitze()
}

def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-s", "--scenario", choices=available_scenarios.keys(), default="LOH1")
    parser.add_argument("-et", "--end-time", type=float, default=None)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-p", "--precision", type=str, default="fp64")
    parser.add_argument("-ut", "--use-tracers", action=argparse.BooleanOptionalAction, default=True)
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
    raise ValueError("The PML ExaSeis application is three-dimensional.")

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns={"v": 3, "sigma": 6, "pml": 27},
    auxiliary_variables={
        "rho": 1, "cp": 1, "cs": 1,
        "dmp_pml": 3, "jacobian": 1,
        "metric_derivative": 9,
        "curve_grid": 3,
    },
    time_step_relaxation=0.9
)

theSolver.add_kernel_optimisations(
    is_linear=True,
    polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto,
    initialise_patches=True,
    precision=args.precision
)

theSolver.set_implementation(
    initial_conditions    = CurvilinearElastic.initial(my_scenario.initial_conditions())+PMLElastic.initial(),
    boundary_conditions   = CurvilinearElastic.boundary(),
    max_eigenvalue        = CurvilinearElastic.eigenvalue(),
    refinement_criterion  = CurvilinearElastic.refinement_criterion(),
    flux                  = PMLElastic.flux(),
    ncp                   = PMLElastic.ncp(),
    material_parameters   = PMLElastic.multiplyMaterialParameterMatrix(),
    riemann_solver        = PMLElastic.riemann_solver(),
    source_term           = PMLElastic.algebraic_source(),
    number_of_point_sources = 1,
    point_source = my_scenario.point_source(),
    init_point_source_location = my_scenario.init_point_source()
)

theSolver._abstract_solver_user_definitions  += CurvilinearElastic.abstractUserDefinitions()
theSolver._abstract_solver_user_declarations += CurvilinearElastic.abstractDeclarations()+PMLElastic.abstractDeclarations()
theSolver._start_grid_initialisation_implementation += CurvilinearElastic.init_grid_step_implementation(my_scenario.domain_perturbations())
theSolver.add_user_solver_includes(PMLElastic.userIncludes())

project.add_solver(theSolver)

if args.use_tracers:
    project.add_action_set_to_timestepping(
    exahype2.tracer.CurviTracer(
        coordinates=my_scenario.tracer_coordinates,
        solver=theSolver,
        filename=args.scenario+"-o"+str(order)+"-"+str(args.precision),
        time_delta_between_two_snapshots=0.01,
        output_precision=10
    ))

project.generate_and_build()
