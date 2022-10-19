
import peano4
import exahype2
import os

import exahype2.solvers.new_ader
from exahype2.solvers.new_ader.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.new_ader.ADERDG import ADERDG
from exahype2.solvers.new_ader.SingleSweep import SingleSweep
from exahype2.solvers.new_ader.ADERDG import Polynomials

project = exahype2.Project( ["exahype2", "elastic"], "curvilinear", ".", executable="LOH1" )

#stability condition: CFL
# dt <= 0.5*dx/lambda_max

unknowns       = {'sigma': 6, 'v': 3}
auxiliary_variables = {'rho': 1, 'cp': 1, 'cs': 1, 'jacobian': 1, 'metric_derivative': 9, 'curve_grid': 3}

max_h = 1.0
min_h          = 0.9 * max_h / 3.0

theSolver=GlobalFixedTimeStep(
  name="CurvilinearSolver",  order=3,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_size=0.001,
  flux=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation,
  ncp=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation,
  point_source=1,
  material_parameters=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation
  )

theSolver.set_implementation( refinement_criterion=exahype2.solvers.new_ader.PDETerms.User_Defined_Implementation)
theSolver.add_kernel_optimizations(is_linear=True,use_kernel_generator=True,architecture="noarch")

project.add_solver(theSolver)

dimensions = 3
build_mode = peano4.output.CompileMode.Release

offset = [-2.28662,  0.0, -2.28662]
size   = [16.3333, 16.3333,  16.3333]

project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = offset,
  size = size,
  min_end_time = 5.0,
  max_end_time = 5.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.1,
  periodic_BC = [False,False,False]
)

#plot everything every 10th time stamp
project.add_plot_filter( offset,size,10 )

#List of probe points every time stamp
probe_points = [ 
  [0.0, 0.0, 0.693],   [0.0, 0.0, 5.543],   [0.0, 0.0, 10.392], 
  [0.490, 0.0, 0.490], [3.919, 0.0, 3.919], [7.348, 0.0, 7.348], 
  [0.577, 0.0, 0.384], [4.612, 0.0, 3.075], [8.647, 0.0, 5.674] ]

#For each of these, add them to the plot filtering and write their output each time stamp
for probe_point in probe_points:
  project.add_plot_filter( probe_point, [0.0,0.0,0.0], 5 )

#If it doesn't already exist, create the directory "output" and later write the solution data there
if not os.path.exists("output"):
  os.mkdir("output")
project.set_output_path( "output" )

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.output.makefile.set_CXX_compiler("mpic++")
peano4_project.output.makefile.add_CXX_flag("-I/home/marc/.local/include")
peano4_project.output.makefile.add_library("-L/home/marc/.local/lib -fopenmp -lcurvi -leasi -lyaml-cpp -limpalajit -lasagi -lnetcdf -lnuma -lmpi")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
