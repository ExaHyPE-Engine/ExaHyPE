import argparse
import os
import sys

import exahype2
from exahype2.solvers.aderdg.ADERDG import Polynomials

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

unknowns = {"v": 3, "sigma": 6, "u": 3, "pml": 27}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "dmp_pml": 3, "jacobian": 1,
    "metric_derivative": 9,
    "curve_grid": 3,
}

offset   = my_scenario.domain_offset
size     = my_scenario.domain_size
order = 5
min_level = 3
max_depth = 0

precision = "fp64"

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

my_scenario.generate_required_files()

project = exahype2.Project(
    ["applications", "exahype2", "exaseis"],
    "pml",
    dimensions=3,
    offset=offset,
    size=size,
    periodic_bc=[False, False, False],
    executable="ExaSeis",
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)
if project.dimensions != 3:
    raise ValueError("The PML dynamic rupture ExaSeis application is three-dimensional.")

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
    initial_conditions    = CurvilinearElastic.initial(my_scenario.initial_conditions())+PMLElastic.initial(),
    boundary_conditions   = CurvilinearElastic.boundary(),
    max_eigenvalue        = CurvilinearElastic.eigenvalue(),
    refinement_criterion  = CurvilinearElastic.refinement_criterion(),
    flux                  = PMLElastic.flux(),
    ncp                   = PMLElastic.ncp(),
    material_parameters   = PMLElastic.multiplyMaterialParameterMatrix(),
    riemann_solver        = DynamicRuptureElastic.riemann_solver(),
    source_term           = PMLElastic.algebraic_source()+PMLDynamicRuptureElastic.algebraic_source()
)

theSolver._abstract_solver_user_declarations += CurvilinearElastic.abstractDeclarations()+DynamicRuptureElastic.abstractDeclarations()+PMLElastic.abstractDeclarations()
theSolver._abstract_solver_user_definitions  += CurvilinearElastic.abstractUserDefinitions()+DynamicRuptureElastic.abstractUserDefinitions()
theSolver._start_grid_initialisation_implementation += CurvilinearElastic.init_grid_step_implementation(my_scenario.domain_perturbations())
theSolver.add_user_solver_includes(PMLDynamicRuptureElastic.userIncludes())

project.add_solver(theSolver)
project.output.makefile.add_h_file("../DynamicRuptureParameters.h", generated=True)

filename=scenario+"-o"+str(order)+"-"+str(precision)
project.add_action_set_to_timestepping(
    exahype2.tracer.CurviTracer(
    coordinates= my_scenario.tracer_coordinates,
    fault_coordinates=my_scenario.fault_coordinates,
    solver=theSolver,
    filename=filename,
    time_delta_between_two_snapshots=0.01,
    output_precision=10,
    unknowns_to_store=12
))

theSolver.add_face_plotter(guard="""
      ((marker.getSelectedFaceNumber() % DIMENSIONS) == repositories::""" + theSolver.get_name_of_global_instance() + """.getFaultNormal()) and
      std::abs(marker.getX()[repositories::""" + theSolver.get_name_of_global_instance() + """.getFaultNormal()]-repositories::""" \
        + theSolver.get_name_of_global_instance() + """.getFaultPosition())< 0.5*marker.getH()[repositories::""" \
        + theSolver.get_name_of_global_instance() + """.getFaultNormal()]
      """)

project.generate_and_build()
