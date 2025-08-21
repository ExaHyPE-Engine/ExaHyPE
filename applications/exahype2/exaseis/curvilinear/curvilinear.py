import os, sys, peano4, exahype2

from exahype2.solvers.aderdg.ADERDG import Polynomials

import os, sys
import subprocess

import CurvilinearElastic

sys.path.insert(0, os.path.abspath("../ExaSeis_core"))
import Scenario

available_scenarios = {
    "LOH1": Scenario.LOH1(),
    "Zugspitze": Scenario.Zugspitze()
}

scenario = "LOH1"
precision = "fp64"
use_tracers = True
order = 5
min_level = 3
plot_dt  = 0.0

scenario_string = scenario.lower()
my_scenario = available_scenarios[scenario]

unknowns = {"v": 3, "sigma": 6}
auxiliary_variables = {
    "rho": 1, "cp": 1, "cs": 1,
    "jacobian": 1, "metric_derivative": 9,
    "curve_grid": 3,
}

offset  = my_scenario.domain_offset
size    = my_scenario.domain_size
end_time = my_scenario.end_time

max_depth = 0
max_h = 1.1 * min(size) / (3.0**min_level)
min_h = max_h / (3.0**max_depth)

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
  initial_conditions    = CurvilinearElastic.initial(my_scenario.initial_conditions()),
  boundary_conditions   = CurvilinearElastic.boundary(),
  max_eigenvalue        = CurvilinearElastic.eigenvalue(),
  refinement_criterion  = CurvilinearElastic.refinement_criterion(),
  flux                  = CurvilinearElastic.flux(),
  ncp                   = CurvilinearElastic.ncp(),
  material_parameters   = CurvilinearElastic.multiplyMaterialParameterMatrix(),
  riemann_solver        = CurvilinearElastic.riemann_solver(),
  number_of_point_sources = 1,
  point_source = my_scenario.point_source(),
  init_point_source_location = my_scenario.init_point_source()
)

theSolver._abstract_solver_user_declarations += CurvilinearElastic.abstractDeclarations()
theSolver._abstract_solver_user_definitions  += CurvilinearElastic.abstractUserDefinitions()
theSolver._start_grid_initialisation_implementation += CurvilinearElastic.init_grid_step_implementation(scenario_string)
theSolver.add_user_solver_includes(CurvilinearElastic.userIncludes())

filename = scenario+"_l"+str(min_level)+"_o"+str(order)+"_pr_"+precision
project = exahype2.Project(["exahype2", "elastic"], "curvi", executable="LOH1")

project.add_solver(theSolver)

if use_tracers:
    project.add_action_set_to_timestepping(
      exahype2.tracer.CurviTracer(
        coordinates=my_scenario.tracer_coordinates,
        solver=theSolver,
        filename=filename,
        data_delta_between_two_snapshots=1e16, time_delta_between_two_snapshots=0.01,
        output_precision=10, clear_database_after_flush=False
    ))

project.set_global_simulation_parameters(
    dimensions=3,
    offset=offset,
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[False, False, False],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project(verbose=False)
peano4_project.build(make_clean_first=True)

os.rename("LOH1", filename)
subprocess.run("make distclean", shell = True, executable="/bin/bash")