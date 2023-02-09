
import peano4
import exahype2

import argparse

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials

project = exahype2.Project( ["examples", "exahype2", "elastic"], "elastic", ".", executable="EL_LIN" )

#stability condition: CFL for Ader-DG
# dt <= c*dx / (lambda_max*(2N+1)
# cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]

unknowns       = {"sigma": 6, "v": 3}
auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}

max_h          = 0.3
min_h          = 0.9 * max_h / 3.0

theSolver=GlobalFixedTimeStep(
  name="elastic",  order=5,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_size=0.0001,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
  ncp=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation)

theSolver.set_implementation( refinement_criterion=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation)
theSolver.add_kernel_optimizations(is_linear=True,use_kernel_generator=True,architecture="noarch")

project.add_solver(theSolver)

dimensions = 3
build_mode = peano4.output.CompileMode.Release

offset = [0.0, 0.0, 0.0]
size = [1.0,1.0,1.0]

project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = offset[0:dimensions],
  size                  = size[0:dimensions],
  min_end_time          = 0.15,
  max_end_time          = 0.15,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.01,
  periodic_BC           = [False,False,False]
)



#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
