
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
min_level           = 3
max_depth           = 0
order               = 3
e_t                 = 2*sqrt(dimensions) #end_time

precision = "float"
# precision = "half_c++23"
# precision = "bfloat"
# precision = "double"
put_data_on_heap = False

offset              = [-1.0, -1.0, -1.0][0:dimensions]
size                = [2.0, 2.0, 2.0][0:dimensions]
unknowns            = 4
auxiliary_variables = 0
max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

project = exahype2.Project( ["exahype2", "acoustic"], ".", executable="PlanarAcoustic_level_"+str(min_level)+"_order_"+str(order) )

theSolver=GlobalAdaptiveTimeStep(
  name="acousticSolver",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
  )


theSolver.add_kernel_optimizations(
  is_linear=True
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
  double k0 = 1.0;
  double k1 = 1.0;
  double k2 = 1.0;

  double w = 6*M_PI/std::sqrt(Dimensions);

  double val = cos( w*t - std::numbers::pi*( 
    k0*x[0]
    + k1*x[1]
#if Dimensions==3
    + k2*x[2]
#endif
  ));

  sol[0] = (k0+k1+k2)*2.0*val;
  sol[1] = k0*val;
  sol[2] = k1*val;
  sol[3] = k2*val;
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


