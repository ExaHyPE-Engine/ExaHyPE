import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimension         = 2
width             = [10,10]
offset            = [0,0]
end_time          = 6.0
plot_interval     = 0.05
patch_size        = 17
maximum_cell_size = 0.002
minimum_cell_size = 0.0001
space_order       = 2
time_order        = 2

solvers = {
  "fv-rusanov-global-fixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
  "fv-rusanov-global-fixed-enclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "fv-rusanov-global-adaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "fv-rusanov-global-adaptive-enclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
  #"fv-rusanov-local-enclave":exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking,
  "rkdg":exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep,
}
solver="fv-rusanov-global-adaptive-enclave"

build_modes = { 
  "release":peano4.output.CompileMode.Release,
  "trace":peano4.output.CompileMode.Trace,
  "assert":peano4.output.CompileMode.Asserts,
  "stats":peano4.output.CompileMode.Stats,
  "debug":peano4.output.CompileMode.Debug,
}
build_mode = "release"

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Acoustic Wave Application Script')
parser.add_argument("-d",       "--dim",               dest="dim",            type=int, default=dimension, help="Number of space dimensions")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int, default=patch_size, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-cs-max",  "--max-cell-size",     dest="max_cell_size",  type=float, default=maximum_cell_size, help="Maximum size of a single finite volume cell on each axis")
parser.add_argument("-cs-min",  "--min-cell-size",     dest="min_cell_size",  type=float, default=minimum_cell_size, help="Minimum size of a single finite volume cell on each axis")
parser.add_argument("-so",      "--space-order",       dest="space_order",    type=int, default=space_order, help="Order of space discretization")
parser.add_argument("-to",      "--time-order",        dest="time_order",     type=int, default=time_order, help="Order of time discretization")
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float, default=plot_interval, help="Time between plots")
parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float, default=end_time, help="End time of simulation")
parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(), default=solver, help="|".join(solvers.keys()) )
parser.add_argument("-m",       "--build-mode",        dest="build_mode",     choices=build_modes.keys(), default=build_mode, help="|".join(build_modes.keys()) )
args = parser.parse_args()

unknowns = {
  "p":1,
  "v":args.dim,
}
parameters = {
  "rho":2.7,
  "c":6.0,
  "sigma":0.1149,
  "t0":0.7,
  "M0":1000.0,
}

initial_conditions = """
  Q[p] = 0.0;
  Q[v + 0] = 0.0;
  Q[v + 1] = 0.0;
"""
boundary_conditions = """
  Qoutside[p] = 0.0;
  Qoutside[v + 0] = 0.0;
  Qoutside[v + 1] = 0.0;
"""
flux = """
  const double K0 = rho * c * c;
  if (normal == 0) {
    F[p] = -K0 * Q[v + 0];
    F[v + 0] = -1.0 / rho * Q[p];
    F[v + 1] = 0.0;
  } else {
    F[p] = -K0 * Q[v + 1];
    F[v + 0] = 0.0;
    F[v + 1] = -1.0 / rho * Q[p];
  }
"""
eigenvalues = """
  return c;
"""
source_term = """
  if (tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>{5, 5}) < 0.1) {
    const double f = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * sigma * sigma));
    S[p] = f;
    S[v + 0] = 0.0;
    S[v + 1] = 0.0;
  }
"""
refinement_criterion = """
  auto command = ::exahype2::RefinementCommand::Keep;
  if (tarch::la::equals(t, 0.0)) {
    double distance = tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>{5, 5});
    if (0.8 < distance && distance < 1.0) {
      command = ::exahype2::RefinementCommand::Refine;
    }
  }
  return command;
"""

if args.dim not in [2,3]:
    print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
    sys.exit(1)

solver_name = "AcousticWaveSolver"
my_solver = solvers[args.solver](
    name                      = solver_name,
    patch_size                = args.patch_size,
    unknowns                  = sum(unknowns.values()),
    auxiliary_variables       = 0,
    min_volume_h              = args.min_cell_size,
    max_volume_h              = args.max_cell_size,
    time_step_relaxation      = 0.01,
    pde_terms_without_state  = True
    #rk_order                 = args.time_order,
    #polynomials              = exahype2.solvers.GaussLegendreBasis(args.space_order),
    #face_projections         = exahype2.solvers.rkdg.FaceProjections.Solution,
)

my_solver.set_implementation(
  initial_conditions   = initial_conditions,
  boundary_conditions  = boundary_conditions,
  flux                 = flux,
  eigenvalues          = eigenvalues,
  source_term          = source_term,
  #refinement_criterion = refinement_criterion
)

index = 0
for key, value in unknowns.items():
  my_solver.add_solver_constants("static constexpr int {} = {};".format(key, index))
  index += value

for key, value in parameters.items():
  my_solver.add_solver_constants("static constexpr double {} = {};".format(key, value))

my_project = exahype2.Project( ["applications","exahype2", "acousticwave"], "AcousticWaveApplication", ".", executable="acoustic-wave-application-" + str(args.dim) + "d-patch-size-" + str(args.patch_size) + "-cell-size-" + str(args.max_cell_size))
my_project.add_solver( my_solver )

my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )

my_project.set_global_simulation_parameters(
  dimensions            = args.dim,
  size                  = width,
  offset                = offset,
  min_end_time          = args.end_time,
  max_end_time          = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_interval,
  periodic_BC           = [False, False, False]
)

#tracer_particles = my_project.add_tracer( name="Tracer", attribute_count=sum(unknowns.values()) )
#my_project.add_action_set_to_initialisation(exahype2.tracer.InsertParticlesByCoordinates( particle_set=tracer_particles, coordinates=[[0.2,0.2]]))
#my_project.add_action_set_to_timestepping(peano4.toolbox.particles.UpdateParallelState(particle_set=tracer_particles))
#my_project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(
#  tracer_particles,
#  my_solver,
#  time_stepping_kernel="toolbox::particles::staticPosition",
#  project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
#))
#my_project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
#  particle_set=tracer_particles,
#  solver=my_solver,
#  filename="Tracer-" + solver_name,
#  data_delta_between_two_snapsots=1e16,
#  time_delta_between_two_snapsots=0.05
#))

build_mode = build_modes[args.build_mode]
my_project.set_Peano4_installation("../../../", build_mode )

peano4_project = my_project.generate_Peano4_project(verbose=True)
peano4_project.generate(throw_away_data_after_generation=True)
peano4_project.build(make_clean_first=True)

print("Done")
