
import peano4
import exahype2
import os

import exahype2.solvers.aderdg
from exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep import GlobalFixedTimeStep
from exahype2.solvers.aderdg.ADERDG import ADERDG
from exahype2.solvers.aderdg.SingleSweep import SingleSweep
from exahype2.solvers.aderdg.ADERDG import Polynomials

project = exahype2.Project( ["exahype2", "elastic"], "curvilinear", ".", executable="LOH1" )

#stability condition: CFL for Ader-DG
# dt <= c*dx / (lambda_max*(2N+1)
# here lambda_max = 77.76
# cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]
# for order 3 dt <= 0.1*3.0/(77.76*(2*3+1)) = 0.000165343915344
# for order 5 dt <= 0.045*3.0/(77.76*(2*5+1)) = 0.000105218855219
# for order 7 dt <= 0.03*3.0/(77.76*(2*7+1)) = 0.0000771604938272


unknowns       = {'sigma': 6, 'v': 3}
auxiliary_variables = {'rho': 1, 'cp': 1, 'cs': 1, 'jacobian': 1, 'metric_derivative': 9, 'curve_grid': 3}

max_h = 3.0
min_h = 0.9 * max_h / 3.0
order = 5
timestep = 0.00005

theSolver=GlobalFixedTimeStep(
  name="CurvilinearSolver",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_size=timestep,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
  ncp=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
  point_source=1,
  material_parameters=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
  )

theSolver.set_implementation( refinement_criterion=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation)
theSolver.add_kernel_optimizations(is_linear=True,use_kernel_generator=True,architecture="noarch", initialize_patches=True)

project.add_solver(theSolver)


tracer_particles = project.add_tracer( name="Tracer", attribute_count=25 )
project.add_action_set_to_initialisation( 
  exahype2.tracer.InsertParticlesByCoordinates( 
    particle_set=tracer_particles,
    coordinates=[
      [0.000, 0.000, 0.693], [0.000, 0.000, 5.543], [0.000, 0.000, 10.392], 
      [0.000, 0.490, 0.490], [0.000, 3.919, 3.919], [0.000, 7.348, 7.3480], 
      [0.000, 0.577, 0.384], [0.000, 4.612, 3.075], [0.000, 8.647, 5.6740] ]
    )
  )
project.add_action_set_to_timestepping(
  peano4.toolbox.particles. UpdateParallelState(
    particle_set=tracer_particles
  )
)

project.add_action_set_to_timestepping(exahype2.tracer.DiscontinuousGalerkinTracing(tracer_particles,
  theSolver,
  time_stepping_kernel="toolbox::particles::staticPosition",
  project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"))

project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
  particle_set=tracer_particles,
  solver=theSolver,
  filename="Tracer-Curvilinear",
  data_delta_between_two_snapsots=1e16,
  time_delta_between_two_snapsots=100*timestep
  ))


dimensions = 3
build_mode = peano4.output.CompileMode.Release

offset = [-2.28662,  0.0, -2.28662]
size   = [16.3333, 16.3333,  16.3333]

project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = offset,
  size = size,
  min_end_time = 0.2,
  max_end_time = 0.2,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.000,
  periodic_BC = [False,False,False]
)


#List of probe points every time stamp
probe_points = [ 
  [0.0, 0.0, 0.693],   [0.0, 0.0, 5.543],   [0.0, 0.0, 10.392], 
#  [0.490, 0.0, 0.490], [3.919, 0.0, 3.919], [7.348, 0.0, 7.348], 
  [0.577, 0.0, 0.384], [4.612, 0.0, 3.075], [8.647, 0.0, 5.674] ]

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveBiPartition", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.output.makefile.set_CXX_compiler("mpic++")
peano4_project.output.makefile.add_library("-fopenmp -lcurvi -leasi -lyaml-cpp -limpalajit -lasagi -lnetcdf -lnuma -lmpi")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
