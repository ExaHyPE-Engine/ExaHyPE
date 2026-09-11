import argparse
import os, sys

import exahype2

sys.path.insert(0, os.path.abspath("../curvilinear"))

import CurvilinearElastic
import DynamicRuptureElastic

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

def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-s", "--scenario", choices=available_scenarios.keys(), default="TPV5")
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
    "dynamicRupture",
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
    raise ValueError("The dynamic rupture ExaSeis application is three-dimensional.")

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns={"v": 3, "sigma": 6, "u": 3},
    auxiliary_variables={
      "rho": 1, "cp": 1, "cs": 1, "jacobian": 1,
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
    initial_conditions    = CurvilinearElastic.initial(my_scenario.initial_conditions()),
    boundary_conditions   = CurvilinearElastic.boundary(),
    max_eigenvalue        = CurvilinearElastic.eigenvalue(),
    flux                  = CurvilinearElastic.flux(),
    ncp                   = CurvilinearElastic.ncp(),
    material_parameters   = CurvilinearElastic.multiplyMaterialParameterMatrix(),
    source_term           = DynamicRuptureElastic.algebraic_source(),
    riemann_solver        = DynamicRuptureElastic.riemann_solver()
)

theSolver._abstract_solver_user_declarations += CurvilinearElastic.abstractDeclarations()+DynamicRuptureElastic.abstractDeclarations()
theSolver._abstract_solver_user_definitions  += CurvilinearElastic.abstractUserDefinitions()+DynamicRuptureElastic.abstractUserDefinitions()
theSolver._start_grid_initialisation_implementation += CurvilinearElastic.init_grid_step_implementation(my_scenario.domain_perturbations())
theSolver.add_user_solver_includes(DynamicRuptureElastic.userIncludes())

project.add_solver(theSolver)
project.output.makefile.add_h_file("../DynamicRuptureParameters.h", generated=True)

project.add_action_set_to_timestepping(
  exahype2.tracer.CurviTracer(
    coordinates= my_scenario.tracer_coordinates,
    fault_coordinates=my_scenario.fault_coordinates,
    solver=theSolver,
    filename=args.scenario+"-o"+str(order)+"-"+str(args.precision),
    time_delta_between_two_snapshots=0.01,
    output_precision=10
))

theSolver.add_face_plotter(guard="""
      ((marker.getSelectedFaceNumber() % DIMENSIONS) == repositories::""" + theSolver.get_name_of_global_instance() + """.getFaultNormal()) and
      std::abs(marker.getX()[repositories::""" + theSolver.get_name_of_global_instance() + """.getFaultNormal()]-repositories::""" \
        + theSolver.get_name_of_global_instance() + """.getFaultPosition())< 0.5*marker.getH()[repositories::""" \
        + theSolver.get_name_of_global_instance() + """.getFaultNormal()]
      """)

project.generate_and_build()
