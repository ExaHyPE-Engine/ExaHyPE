import os, sys
import peano4
import exahype2
import peano4.toolbox.particles

project = exahype2.Project( ["applications","exahype2", "AcousticWave"], "AcousticWaveFV", ".", executable="AcousticWaveFV" )

solver_name = "AcousticWaveFV"
unknowns    = 3
auxiliary_variables = 0
mesh_size   = 0.3
patch_size  = 10
dimensions  = 2

thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  solver_name,
  patch_size,
  unknowns, auxiliary_variables,
  mesh_size, 
  mesh_size,
  time_step_relaxation = 0.01,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  source_term = exahype2.solvers.fv.PDETerms.User_Defined_Implementation, 
  eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

project.add_solver( thesolver )

tracer_particles = project.add_tracer( name="Tracer",attribute_count=59 )
project.add_action_set_to_initialisation( 
  exahype2.tracer.InsertParticlesbyCoordinates( 
    particle_set=tracer_particles, coordinates=[[2.5,0.0],[5.0,5.0],[7.5,0.0]]))
project.add_action_set_to_timestepping(
  peano4.toolbox.particles.UpdateParallelState(
    particle_set=tracer_particles
  )
)
project.add_action_set_to_timestepping(
  exahype2.tracer.FiniteVolumesTracing(
    tracer_particles, 
    thesolver, 
    time_stepping_kernel="toolbox::particles::staticPosition",
    project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
  )
)

project.add_action_set_to_timestepping(
  exahype2.tracer.DumpTrajectoryIntoDatabase(
    particle_set=tracer_particles,
    solver=thesolver,
    filename="Tracer-" + solver_name,
    data_delta_between_two_snapsots=1e16,
    time_delta_between_two_snapsots=0.0001
  )
)

build_mode = peano4.output.CompileMode.Release
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0],
  size = [10.0,10.0],
  min_end_time = 5.0,
  max_end_time = 5.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.1,
  periodic_BC           = [False, False, False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation("../../../", build_mode )
peano4_project = project.generate_Peano4_project(False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=12)
