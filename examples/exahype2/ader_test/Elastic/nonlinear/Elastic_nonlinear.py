
import peano4
import exahype2

import argparse

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials

project = exahype2.Project( ["examples", "exahype2", "elastic"], "elastic", ".", executable="NL_EL" )

#stability condition: CFL for Ader-DG
# dt <= c*dx / (lambda_max*(2N+1)
# cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]

unknowns       = {"sigma": 6, "v": 3} 
auxiliary_variables = { "rho":1, "cp": 1, "cs": 1}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - ADER testing script')
parser.add_argument("-cs", "--cell-size",              dest="h",              type=float, default=0.3, help="Mesh size" )
parser.add_argument("-d",  "--dim",                    dest="dim",            type=int, default=2, help="Dimensions" )
args = parser.parse_args()

max_h          = args.h
min_h          = 0.9 * max_h / 3.0
time_step_size=0.00001

theSolver=GlobalFixedTimeStep(
  name="elastic",  order=3,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_size=time_step_size,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
  ncp=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
  )

theSolver.set_implementation( refinement_criterion=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation)
theSolver.add_kernel_optimizations(is_linear=False,use_kernel_generator=True,architecture="noarch")

project.add_solver(theSolver)

dimensions = args.dim
build_mode = peano4.output.CompileMode.Release

end_time = 2000*time_step_size

project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0,0.0][0:dimensions],
  size = [1.0,1.0,1.0][0:dimensions],
  min_end_time = end_time,
  max_end_time = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 50*time_step_size,
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
