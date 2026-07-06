import argparse
import os
import sys

import peano4
import exahype2
from exahype2.solvers.aderdg.ADERDG import Polynomials

sys.path.insert(0, os.path.abspath("../pml"))

import PMLElastic
import DynamicRuptureElastic

#sys.path.insert(0, os.path.abspath("../ExaSeis_core/Scenario"))
#import TPV5, TPV26, TPV28, TPV29

sys.path.insert(0, os.path.abspath("../ExaSeis_core"))
import Scenario


available_scenarios = {
    "TPV5":     Scenario.TPV5(),
    "TPV6":     Scenario.TPV6(),
    "TPV16":    Scenario.TPV16(),
    "TPV26":    Scenario.TPV26(),
    "TPV28":    Scenario.TPV28(),
    "TPV29":    Scenario.TPV29(),
    "TPV34":    Scenario.TPV34(),
    "Husavik":  Scenario.Husavik()
}


scenario = "TPV5"
scenario_string = scenario.lower()
my_scenario = available_scenarios[scenario]

unknowns = {"v": 3, "sigma": 6, "u": 3}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1, "jacobian": 1, 
    "metric_derivative": 9,
    "curve_grid": 3,
}

offset  = my_scenario.domain_offset
size    = my_scenario.domain_size

order = 5
min_level = 3
max_depth = 0

precision = "fp32"

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
    parser.add_argument("-l", "--grid-levels", type=int, default=min_level)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()
order = args.degrees_of_freedom
end_time = args.end_time

my_scenario.generate_required_files(order)

filename=scenario+"_l_"+str(args.grid_levels)+"_o_"+str(order)+"_pr_"+str(precision)
project = exahype2.Project(
    ["applications", "exahype2", "exaseis"],
    "dynamicRupture",
    dimensions=3,
    offset=offset,
    size=size,
    periodic_bc=[False, False, False],
    executable=filename,
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 3:
    raise ValueError("The dynamic rupture ExaSeis application is three-dimensional.")

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
    precision=precision
)

theSolver.set_implementation(
    initial_conditions    = DynamicRuptureElastic.initial(my_scenario.initial_conditions()), #same as curvi, which still need to be templated for initial conditions
    boundary_conditions   = PMLElastic.boundary(),
    max_eigenvalue        = PMLElastic.eigenvalue(),
    flux                  = DynamicRuptureElastic.flux(), #same as curvi
    ncp                   = DynamicRuptureElastic.ncp(), # same as curvi
    material_parameters   = DynamicRuptureElastic.multiplyMaterialParameterMatrix(), #same as curvi +u term
    source_term           = DynamicRuptureElastic.algebraic_source(),
    riemann_solver        = DynamicRuptureElastic.riemann_solver()
)

theSolver._abstract_solver_user_declarations += DynamicRuptureElastic.abstractDeclarations()
theSolver._abstract_solver_user_definitions  += DynamicRuptureElastic.abstractUserDefinitions()
theSolver._start_grid_initialisation_implementation += DynamicRuptureElastic.init_grid_step_implementation(scenario_string)
theSolver._destructor_implementation +=  DynamicRuptureElastic.abstractDestructor()
theSolver.add_user_solver_includes(DynamicRuptureElastic.userIncludes())

project.add_solver(theSolver)

project.add_action_set_to_timestepping(
  exahype2.tracer.CurviTracer(
    coordinates= my_scenario.tracer_coordinates,
    fault_coordinates=my_scenario.fault_coordinates,
    solver=theSolver,
    filename=filename,
    data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
    output_precision=10, clear_database_after_flush=False,
    number_of_entries_between_two_db_flushes=100
))

theSolver.add_face_plotter(guard="""
      ((marker.getSelectedFaceNumber() % DIMENSIONS) == 2-repositories::""" + theSolver.get_name_of_global_instance() + """.context->fault_normal) and
      std::abs(marker.getX()[2-repositories::""" + theSolver.get_name_of_global_instance() + """.context->fault_normal]-repositories::""" \
        + theSolver.get_name_of_global_instance() + """.context->fault_position)< 0.5*marker.getH()[2-repositories::""" \
        + theSolver.get_name_of_global_instance() + """.context->fault_normal]
      """)

project.generate_and_build()
