import os
import sys
import argparse
import sympy

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimension         = 2
width             = [10,10]
offset            = [0,0]
end_time          = 6.0
plot_interval     = 0.05
patch_size        = 1
maximum_cell_size = 1
minimum_cell_size = 0.005
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
parser.add_argument("--no-compile",                    dest="compile",        action="store_false", default="True", help="Compile (on by default)" )
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
    sys.exit(1)

pde = exahype2.symhype.FirstOrderConservativePDEFormulation(unknowns=args.dim+1, auxiliary_variables=0, dimensions=args.dim)
p = pde.name_Q_entry(0, "p")
v = pde.name_Q_entries(1, args.dim, "v")

pde.initial_values[0] = 0
pde.initial_values[1] = 0
pde.initial_values[2] = 0

pde.boundary_values[0] = 0
pde.boundary_values[1] = 0
pde.boundary_values[2] = 0

rho = sympy.symbols("rho")
c = sympy.symbols("c")
K0 = rho * c * c

# [unknowns, dimensions]
pde.F[0, 0] = -K0 * v[0]
pde.F[0, 1] = -K0 * v[1]
pde.F[1, 0] = -1/rho * p
pde.F[1, 1] = 0
pde.F[2, 0] = 0
pde.F[2, 1] = -1/rho * p

pde.eigenvalues[0] = c
pde.eigenvalues[1] = -c
pde.eigenvalues[2] = 0

sigma = sympy.symbols("sigma")
t0 = sympy.symbols("t0")
M0 = sympy.symbols("M0")
t = sympy.symbols("t")
f = M0 * sympy.exp(-((t - t0) * (t - t0)) / (2 * sigma * sigma))
pde.sources[0] = f
pde.sources[1] = 0
pde.sources[2] = 0

solver_name = "AcousticWaveSolver"
my_solver = solvers[args.solver](
    name                      = solver_name,
    patch_size                = args.patch_size,
    unknowns                  = args.dim+1,
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
  initial_conditions   = pde.implementation_of_initial_conditions(),
  boundary_conditions  = pde.implementation_of_boundary_conditions(),
  flux                 = pde.implementation_of_flux(),
  eigenvalues          = pde.implementation_of_max_eigenvalue(),
  source_term          = pde.implementation_of_sources(),
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
)

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
if(args.compile):
  peano4_project.build(make_clean_first=True, throw_away_data_after_build=True)

print("PDE:\n" + str(pde))
print("Done")
