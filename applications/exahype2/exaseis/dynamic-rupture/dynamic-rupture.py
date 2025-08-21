import os, sys, peano4, exahype2

from exahype2.solvers.aderdg.ADERDG import Polynomials

sys.path.insert(0, os.path.abspath("../pml"))

import PMLElastic
import DynamicRuptureElastic

#sys.path.insert(0, os.path.abspath("../ExaSeis_core/Scenario"))
#import TPV5, TPV26, TPV28, TPV29

sys.path.insert(0, os.path.abspath("../ExaSeis_core"))
import Scenario


available_scenarios = {
    "TPV5": Scenario.TPV5(),
    "TPV26": Scenario.TPV26(), 
    "TPV28": Scenario.TPV28(),
    "TPV29": Scenario.TPV29()
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

plot_dt  = 0.0
end_time = my_scenario.end_time

order = 5
min_level = 3
max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

precision = "fp32"

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
    riemann_solver=DynamicRuptureElastic.riemann_solver()
)

theSolver._finish_time_step_implementation   += DynamicRuptureElastic.finish_time_step_implementation(scenario_string)
theSolver._abstract_solver_user_declarations += DynamicRuptureElastic.abstractDeclarations()
theSolver._abstract_solver_user_definitions  += DynamicRuptureElastic.abstractUserDefinitions()
theSolver._start_grid_initialisation_implementation += DynamicRuptureElastic.init_grid_step_implementation(scenario_string)
theSolver._destructor_implementation +=  DynamicRuptureElastic.abstractDestructor()
theSolver.add_user_solver_includes(DynamicRuptureElastic.userIncludes())

filename=scenario+"_l_"+str(min_level)+"_o_"+str(order)+"_pr_"+str(precision)
project = exahype2.Project(["exahype2", "elastic"], "dynamicRupture", executable=filename)
project.add_solver(theSolver)

project.add_action_set_to_timestepping(
    exahype2.tracer.CurviTracer(
    coordinates= my_scenario.tracer_coordinates,
    solver=theSolver,
    filename=filename,
    data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
    output_precision=10, clear_database_after_flush=False
))

if plot_dt > 0.0:
  project.set_output_path("solutions"+filename)

project.set_global_simulation_parameters(
    dimensions=3,
    offset=offset,
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots = plot_dt,
    periodic_BC=[False, False, False]
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make_clean_first=True)
