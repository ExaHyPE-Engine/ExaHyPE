
import peano4
import exahype2
import os
from math import sqrt

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep import GlobalAdaptiveTimeStep
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials

dimensions          = 2
min_level           = 2
max_depth           = 0
order               = 3
e_t                 = 2.0 #end_time

# precision = "float"
# precision = "float16_t"
# precision = "bfloat16_t"
precision = "double"
put_data_on_heap = False

offset              = [-1.0, -1.0, -1.0][0:dimensions]
size                = [2.0, 2.0, 2.0][0:dimensions]
unknowns            = {"rho": 1, "rho_u": 3, "E": 1}
auxiliary_variables = {}
max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

project = exahype2.Project( ["exahype2", "euler"], ".", executable="PlanarEuler_level_"+str(min_level)+"_order_"+str(order) )

theSolver=GlobalAdaptiveTimeStep(
  name="eulerSolver",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
  )


theSolver.add_kernel_optimizations(
  is_linear=False
  ,use_kernel_generator=True
  ,architecture="noarch"
  ,hold_cell_data_on_heap_with_smart_pointer = put_data_on_heap
  ,hold_face_data_on_heap_with_smart_pointer = put_data_on_heap
  ,solution_persistent_storage_precision=precision
  ,predictor_computation_precisions=[precision]
  ,corrector_computation_precision=precision
)

theSolver.set_implementation( refinement_criterion="return ::exahype2::RefinementCommand::Keep;")
theSolver.add_error_measurement(error_measurement_implementation="""

  sol[0] = 0.02*(1+std::exp(-0.5*tarch::la::norm2(x)/0.01));

  sol[1] = 1.0*sol[0];
  sol[2] = 1.0*sol[0];
  sol[3] = 0.0;

  constexpr double gamma = 1.4;
  const double p = 1.0;

  //Should lead to a constant p over the domain given the initial conditions
  sol[4] = p/(gamma-1.0) + 0.5*(sol[1]*sol[1]+sol[2]*sol[2]+sol[3]*sol[3]) / sol[0];

"""
)

project.add_solver(theSolver)

build_mode = peano4.output.CompileMode.Release

project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = offset[0:dimensions],
  size = size[0:dimensions],
  min_end_time = e_t,
  max_end_time = e_t,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC = [True, True, True]
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::SpreadOutHierarchically", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=32)


