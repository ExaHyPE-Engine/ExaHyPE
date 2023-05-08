import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimensions            = 2
width                 = [20, 20]
offset                = [-5, 0]
source_location       = [10, 10]
tracer_location       = [0.2, 0.2]
end_time              = 0.02
time_step_relaxation  = 0.99
plot_interval         = 0.001
patch_size            = 17
maximum_cell_size     = 0.1
amr_levels            = 3

solvers = {
  #"FVRusanovGlobalFixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
  #"FVRusanovGlobalFixedEnclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "FVRusanovGlobalAdaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "FVRusanovGlobalAdaptiveEnclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
  #"FVRusanovLocalEnclave":exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking,
}

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Elastic Wave Application Script')
parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(),     required=True,              help="|".join(solvers.keys()) )
parser.add_argument("-m",       "--build-mode",        dest="build_mode",     choices=build_modes.keys(), default="Release",          help="|".join(build_modes.keys()) )
parser.add_argument("-d",       "--dim",               dest="dim",            type=int,                   default=dimensions,         help="Number of space dimensions")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int,                   default=patch_size,         help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-maxh",    "--max-cell-size",     dest="max_cell_size",  type=float,                 default=maximum_cell_size,  help="Maximum size of a single finite volume cell on each axis")
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float,                 default=plot_interval,      help="Time between plots")
parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float,                 default=end_time,           help="End time of simulation")
parser.add_argument("-amr",     "--amr-levels",        dest="amr_levels",     type=int,                   default=amr_levels,         help="Number of AMR grid levels on top of max. finite volume cell (3 by default)" )
parser.add_argument(            "--dynamic-amr",       dest="dynamic_amr",    action="store_true",        default=False,              help="Use dynamic AMR" )
parser.add_argument("-gpu",     "--gpu",               dest="gpu",            action="store_true",        default=False,              help="Enable GPU support (stateless PDE terms). Requires a GPU build." )
parser.add_argument("-tracer",  "--tracer",            dest="tracer",         action="store_true",        default=False,              help="Use particle tracer" )
parser.add_argument(            "--no-compile",        dest="compile",        action="store_false",       default=True,               help="Compile (on by default)" )
args = parser.parse_args()

from pde import PDE
my_pde = PDE(dimensions=args.dim, auxiliary_variables=0)
my_pde.substitute_expression(my_pde.rho, 2.7)
my_pde.substitute_expression(my_pde.cp, 6.0)
my_pde.substitute_expression(my_pde.cs, 3.464)
my_pde.substitute_expression(my_pde.t0, 0.7)
my_pde.substitute_expression(my_pde.M0, 1000.0)

solver_type = args.solver
if args.gpu:
  if not "Enclave" in solver_type:
    solver_type += "Enclave"
solver_name = "ElasticWaveSolver" + str(solver_type)
if args.gpu:
  solver_name += "GPU"
my_solver = solvers[solver_type](
  name                      = solver_name,
  patch_size                = args.patch_size,
  unknowns                  = my_pde.unknowns,
  auxiliary_variables       = my_pde.auxiliary_variables,
  max_volume_h              = args.max_cell_size,
  min_volume_h              = args.max_cell_size * 3.0**(-args.amr_levels),
  time_step_relaxation      = time_step_relaxation
)

if args.gpu:
  my_solver.set_pde_terms_without_state(True)

my_solver.add_solver_constants("static constexpr bool useDynamicAMR = {};".format("true" if args.dynamic_amr else "false"))
my_solver.add_solver_constants("static constexpr double sourceLocation[] = {%s};" % str(source_location).strip('[]'))
my_solver.set_implementation(
  initial_conditions    = my_pde.implementation_of_initial_conditions(),
  boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
  flux                  = my_pde.implementation_of_flux(),
  eigenvalues           = my_pde.implementation_of_max_eigenvalue(),
  source_term           = """
  if (tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>(sourceLocation)) < 0.1) {
    %s
  }
""" % my_pde.implementation_of_sources(),
  refinement_criterion  = """
  const double distance = tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>(sourceLocation));
  auto command = tarch::la::equals(t, 0.0)
    ? (0.8 < distance && distance < 1.0) ? ::exahype2::RefinementCommand::Refine : ::exahype2::RefinementCommand::Keep
    : ::exahype2::RefinementCommand::Keep;

  if constexpr (useDynamicAMR) {
    if (tarch::la::greater(t, 0.0)) {
      if (Q[4]>  0.4) {
        command = ::exahype2::RefinementCommand::Refine;
      }
      if (Q[4] < 0.2) {
        command = ::exahype2::RefinementCommand::Erase;
      }
    }
  }

  return command;
"""
)

my_project = exahype2.Project( ["applications", "exahype2", "elasticwave"], "ElasticWaveApplication", ".", executable="elastic-wave-application-" + str(args.dim) + "d-patch-size-" + str(args.patch_size) + "-cell-size-" + str(args.max_cell_size))
my_project.add_solver( my_solver )
my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", build_modes[args.build_mode])

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

if args.tracer:
  tracer_particles = my_project.add_tracer( name="Tracer", attribute_count=args.dim + 1 )
  my_project.add_action_set_to_initialisation(exahype2.tracer.InsertParticlesByCoordinates( particle_set=tracer_particles, coordinates=[tracer_location]))
  my_project.add_action_set_to_timestepping(peano4.toolbox.particles.UpdateParallelState(particle_set=tracer_particles))
  my_project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(
    tracer_particles,
    my_solver,
    time_stepping_kernel="toolbox::particles::staticPosition",
    project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
  ))
  my_project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
    particle_set=tracer_particles,
    solver=my_solver,
    filename="Tracer-" + solver_name,
    data_delta_between_two_snapsots=1e16,
    time_delta_between_two_snapsots=0.05
  ))

peano4_project = my_project.generate_Peano4_project(verbose=True)
peano4_project.generate()
if(args.compile):
  peano4_project.build(make_clean_first=True, throw_away_data_after_build=True)
