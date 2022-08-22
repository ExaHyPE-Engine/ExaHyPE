
import peano4
import exahype2

import argparse

import exahype2.solvers.new_ader
from exahype2.solvers.new_ader.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.new_ader.ADERDG import ADERDG
from exahype2.solvers.new_ader.SingleSweep import SingleSweep
from exahype2.solvers.new_ader.ADERDG import Polynomials

project = exahype2.Project( ["examples", "exahype2", "elastic"], "elastic", ".", executable="EL_LIN" )

#stability condition: CFL
# dt <= 0.5*dx/lambda_max

unknowns       = 8
auxiliary_variables = 0
max_h          = 0.1
min_h          = 0.9 * max_h / 3.0

theSolver=GlobalFixedTimeStep(
  name="elastic",  order=3, polynomials=Polynomials.Gauss_Legendre,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_size=0.00001,
  flux=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation,
  ncp=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation,
  is_linear=False)

theSolver.set_implementation( refinement_criterion=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation)

project.add_solver(theSolver)

dimensions = 2
build_mode = peano4.output.CompileMode.Release


project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0,0.0],
  size = [1.0,1.0,1.0],
  min_end_time = 0.05,
  max_end_time = 0.05,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.001,
  periodic_BC = [False,False,False]
)



#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
