
import peano4, exahype2
import os


min_level           = 3
max_depth           = 0
order               = 5
e_t = 2.0


size_1d = 9.0
size    = [size_1d, size_1d, size_1d]   # thousand meters
offset  = [-size[0]/2., -size[1]/2., 0.0]  # thousand meters

unknowns            = {"v": 3, "sigma": 6}
auxiliary_variables = {"rho":1, "cp": 1, "cs": 1}

max_h               = 1.1 * min(size) / (3.0**min_level)
min_h               = max_h / (3.0**max_depth)

project = exahype2.Project( ["examples", "exahype2", "elastic"], ".", executable="LOH" )

theSolver=exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep(
  name="elastic",  order=order,
  unknowns=unknowns, auxiliary_variables=auxiliary_variables,
  min_cell_h=min_h, max_cell_h=max_h, time_step_relaxation=0.9,
  flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
  point_source=1
)

theSolver.add_kernel_optimizations(is_linear=True)

project.add_solver(theSolver)

tracer_particles = project.add_tracer(name="Tracer", attribute_count=12)

project.add_action_set_to_initialisation(
  exahype2.tracer.InsertParticlesByCoordinates(
    particle_set=tracer_particles,
    coordinates=[ [0., 0.693, 0.],    [0., 5.543, 0.],    [0., 10.392, 0.],
                  [0.490, 0.490, 0.], [3.919, 3.919, 0.], [7.348, 7.348, 0.],
                  [.577, .384, 0.],   [4.612, 3.075, 0.], [8.647, 5.764, 0.] ],
  )
)
project.add_action_set_to_timestepping(
  peano4.toolbox.particles.api.UpdateParallelState(particle_set=tracer_particles)
)

project.add_action_set_to_timestepping(
  exahype2.tracer.DiscontinuousGalerkinTracing(
      tracer_particles, theSolver,
      project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle",
  )
)

project.add_action_set_to_timestepping(
  exahype2.tracer.DumpTracerIntoDatabase(
      particle_set=tracer_particles, solver=theSolver,
      filename="tracers/HHS",
      data_delta_between_two_snapsots=1e16, 
      time_delta_between_two_snapsots=0.0,
      output_precision=10
  )
)


if not os.path.exists("tracers"):
  os.makedirs("tracers")


project.set_global_simulation_parameters(
  dimensions            = 3,
  offset                = offset[0:3],
  size                  = size[0:3],
  min_end_time          = e_t,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../", peano4.output.CompileMode.Release )
peano4_project = project.generate_Peano4_project("False")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=16)
