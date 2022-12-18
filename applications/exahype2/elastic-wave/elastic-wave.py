import os, sys
import argparse

import peano4
import exahype2

dimension = 2
width  = [10,10]
offset = [0,0]
end_time = 4.0
plot_interval = 0.1
patch_size = 3
maximum_cell_size = 0.2
minimum_cell_size = 0.06
space_order = 2
time_order = 2
solvers = {
  "fv-rusanov-global-fixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
  "fv-rusanov-global-fixed-enclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "fv-rusanov-global-adaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "fv-rusanov-global-adaptive-enclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
  #"fv-rusanov-local-enclave":exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking,
  "rkdg":exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep,
}
solver="fv-rusanov-global-adaptive"

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Elastic Wave Application Script')
parser.add_argument("-d",       "--dim",               dest="dim",            type=int, default=dimension, help="Number of space dimensions")
parser.add_argument("-v",       "--verbose",           dest="verbose",        action="store_true", default=True, help="Verbose mode")
parser.add_argument(            "--debug",             dest="debug"  ,        action="store_true", default=False, help="Debug mode")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int, default=patch_size, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-cs-max",  "--max-cell-size",     dest="max_cell_size",  type=float, default=maximum_cell_size, help="Size of a single finite volume cell on each axis")
parser.add_argument("-cs-min",  "--min-cell-size",     dest="min_cell_size",  type=float, default=minimum_cell_size, help="Size of a single finite volume cell on each axis")
parser.add_argument("-so",      "--space-order",       dest="space_order",    type=int, default=space_order, help="Order of space discretization")
parser.add_argument("-to",      "--time-order",        dest="time_order",     type=int, default=time_order, help="Order of time discretization")
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float, default=plot_interval, help="Time between plots")
parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float, default=end_time, help="End time of simulation")
parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(), default=solver, help="|".join(solvers.keys()) )
args = parser.parse_args()

unknowns = {
  "v":args.dim,
  "sigma":3,
}
parameters = {
  "rho":2.7,
  "cp":6.0,
  "cs":3.464,
  "t0":0.7,
  "M0":1000.0,
}

initial_conditions = """
  Q[v + 0] = 0.0:
  Q[v + 1] = 0.0;
  Q[sigma + 0] = 0.0;
  Q[sigma + 1] = 0.0;
  Q[sigma + 2] = 0.0;
"""
boundary_conditions = """
    Qoutside[v + 0] = 0.0;
    Qoutside[v + 1] = 0.0;
    Qoutside[sigma + 0] = 0.0;
    Qoutside[sigma + 1] = 0.0;
    Qoutside[sigma + 2] = 0.0;
"""
refinement_criterion = """
  auto command = ::exahype2::RefinementCommand::Keep;
  if (tarch::la::equals(t, 0.0)) {
    double distance = tarch::la::norm2(volumeX - {5.0, 5.0});
    if (0.8 < distance && distance < 1.0) {
      command = ::exahype2::RefinementCommand::Refine;
    }
  }
  return command;
"""
flux = """
  // Lamé parameters
  const double mu = rho * cs * cs;
  const double lambda = rho * cp * cp - 2.0 * mu;

  const double u = Q[v + 0];
  const double v = Q[v + 1];
  const double sxx = Q[sigma + 0];
  const double syy = Q[sigma + 1];
  const double sxy = Q[sigma + 2];
"""
eigenvalues = """
  return cp;
"""
source_term = """
  if (tarch::la::norm2(volumeCentre - {5, 5}) < 0.1) {
    const double f = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * sigma * sigma));
    S[0] = f;
    S[1] = 0.0;
    S[2] = 0.0;
  }
"""

if args.dim not in [2,3]:
    print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
    sys.exit(1)

solver_name = "ElasticWaveSolver"
my_solver = solvers[args.solver](
    name                      = solver_name,
    patch_size                = args.patch_size,
    unknowns                  = sum(unknowns.values()),
    auxiliary_variables       = 0,
    min_volume_h              = args.minimal_cell_size,
    max_volume_h              = args.maximal_cell_size,
    time_step_relaxation      = 0.01,
    #pde_terms_without_state  = True
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
)

for key, value in unknowns.items():
  my_solver.add_solver_constants("static constexpr int {} = {};".format(key, value))

for key, value in parameters.items():
  my_solver.add_solver_constants("static constexpr double {} = {};".format(key, value))

project = exahype2.Project( ["applications","exahype2", "elasticcwave"], "ElasticWaveApplication", ".", executable="elastic-wave-application-" + str(args.dim) + "d-patch-size-" + str(args.patch_size) + "-cell-size-" + str(args.max_cell_size))
project.add_solver( my_solver )

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )

project.set_global_simulation_parameters(
  dimensions            = args.dim,
  size                  = width,
  offset                = offset,
  min_end_time          = args.end_time,
  max_end_time          = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_interval,
  periodic_BC           = [False, False, False]
)

if args.debug:
  build_mode = peano4.output.CompileMode.Debug
else:
  build_mode = peano4.output.CompileMode.Release
project.set_Peano4_installation("../../../", build_mode )

peano4_project = project.generate_Peano4_project(verbose=args.verbose)
peano4_project.generate(throw_away_data_after_generation=True)
peano4_project.build(make_clean_first=True)

print("Done")
