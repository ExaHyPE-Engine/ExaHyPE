import os, sys, peano4, exahype2

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

end_time = my_scenario.end_time
plot_dt  = 0.0

order = 5

min_level = 3
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

precision = "fp64"
use_tracers = True

my_scenario.generate_required_files(order)

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ElasticSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_cell_h=min_h,
    max_cell_h=max_h,
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

project = exahype2.Project(["exahype2", "elastic"], "pml", executable=scenario+"_l_"+str(min_level)+"_o_"+str(order)+"_pr_"+precision)
project.add_solver(theSolver)

if use_tracers:
    project.add_action_set_to_timestepping(
    exahype2.tracer.CurviTracer(
        coordinates=my_scenario.tracer_coordinates,
        solver=theSolver,
        filename=scenario+"_level_"+str(min_level)+"-order-"+str(order)+"-precision-"+str(precision),
        data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
        output_precision=10, clear_database_after_flush=False
    ))

if plot_dt > 0.0:
  project.set_output_path("solutions")

project.set_global_simulation_parameters(
    dimensions=3,
    offset=offset,
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=plot_dt,
    periodic_BC=[False, False, False]
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation("../../../../", peano4.output.CompileMode.Release)
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make_clean_first=True)
