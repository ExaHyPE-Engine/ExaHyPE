
import peano4
import exahype2
import os
from math import sqrt

from exahype2.solvers.aderdg.rusanov.FVLimitingGlobalAdaptiveTimeStep import FVLimitingGlobalAdaptiveTimeStep

dimensions          = 2
min_level           = 2
max_depth           = 0
order               = 5
e_t                 = 2.0 #end_time


put_data_on_heap = False

offset              = [-1.0, -1.0, -1.0][0:dimensions]
size                = [2.0, 2.0, 2.0][0:dimensions]
unknowns            = {"rho": 1, "rho_u": 3, "E": 1}
auxiliary_variables = {}
max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

project = exahype2.Project( ["exahype2", "euler"], ".", executable="EulerSodShock_level_"+str(min_level)+"_order_"+str(order) )

theSolver=FVLimitingGlobalAdaptiveTimeStep(
  name="eulerSolver",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.1,
  number_of_dmp_observables=5,
  dmp_relaxationParameter = 0.02,
  dmp_differences_scaling = 0.04,
  physical_admissibility_criterion=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
)

# theSolver.set_implementation( refinement_criterion="return ::exahype2::RefinementCommand::Keep;")

project.add_solver(theSolver)

build_mode = peano4.output.CompileMode.Release

project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = offset[0:dimensions],
  size = size[0:dimensions],
  min_end_time = e_t,
  max_end_time = e_t,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.1,
  periodic_BC = [False, False, False]
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::strategies::SpreadOutHierarchically", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=32)


