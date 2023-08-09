
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

dimensions = 3

project = exahype2.Project( ["exahype2", "acoustic"], ".", executable="Acoustic"+str(dimensions)+"D" )

#stability condition: CFL for Ader-DG
# dt <= c*dx / (lambda_max*(2N+1)
# cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]
# here lambda_max = 100
# dt <= 0.1 * 0.03 / 700 \approx  0.000004
# dt <= 0.1 * 0.03 / 1100 \approx  0.0000025

unknowns       = 4
auxiliary_variables = 0

max_h = 0.6
min_h = 0.9 * max_h / 3.0
e_t = 2*sqrt(dimensions) #end_time

# timestep = 0.00001

theSolver=GlobalAdaptiveTimeStep(
  name="acousticSolver",  order=3,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9,
  flux=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
  )


theSolver.set_implementation( refinement_criterion=exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation)
theSolver.add_kernel_optimizations(is_linear=True,use_kernel_generator=True,architecture="noarch")

project.add_solver(theSolver)


tracer_particles = project.add_tracer( name="Tracer_acoustic",attribute_count=4 )
init_action_set =  exahype2.tracer.InsertParticlesByCoordinates( 
    particle_set=tracer_particles, coordinates=[[0.0,0.0,0.0][0:dimensions],[0.25,0.25,0.25][0:dimensions],
                                                [0.5,0.5,0.5][0:dimensions],[0.75,0.75,0.75][0:dimensions]])
init_action_set.priority = 0
project.add_action_set_to_initialisation( init_action_set )

tracer_action_set=exahype2.tracer.DiscontinuousGalerkinTracing(
  tracer_particles,
  theSolver,
  project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
)
tracer_action_set.priority = 1
project.add_action_set_to_timestepping(tracer_action_set)
project.add_action_set_to_initialisation(tracer_action_set)

dump_action_set=exahype2.tracer.DumpTracerIntoDatabase(
  particle_set=tracer_particles,
  solver=theSolver,
  filename="Tracer-acoustic",
#  number_of_entries_between_two_db_flushes=1000,
  output_precision=8,
  data_delta_between_two_snapsots=1e16,
  position_delta_between_two_snapsots=1e16,
  time_delta_between_two_snapsots=0.001
  )

dump_action_set.priority = 3
project.add_action_set_to_timestepping(dump_action_set)


build_mode = peano4.output.CompileMode.Release

offset = [-1.0, -1.0, -1.0]
size   = [2.0, 2.0, 2.0]

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
project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", build_mode )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=12)
