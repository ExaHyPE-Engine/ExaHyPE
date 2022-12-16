import os, sys
import argparse

import peano4
import exahype2

print("""

ExaHyPE 2 - Acoustic Wave Application

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Acoustic Wave Finite Volume Application Script')
parser.add_argument("-d",   "--dim",               dest="dim",            type=int, default=2, help="Number of space dimensions")
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose mode")
parser.add_argument(        "--debug",             dest="debug"  ,        action="store_true", default=False, help="Debug mode")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=10, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-cs",  "--cell-size",         dest="cell_size",      type=float, default=0.3, help="Size of a single finite volume cell on each axis")
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
    sys.exit(1)

project = exahype2.Project( ["applications","exahype2", "acousticwave"], "AcousticWaveApplication", ".", executable="acoustic-wave-application-" + str(args.dim) + "d-patch-size-" + str(args.patch_size) + "-cell-size-" + str(args.cell_size))

unknowns    = 3
auxiliary_variables = 0

fv_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name="AcousticWaveFVRusanovSolver",
  patch_size=args.patch_size,
  unknowns=3,
  auxiliary_variables=0,
  min_volume_h=args.cell_size,
  max_volume_h=args.cell_size,
  time_step_relaxation = 0.01,
  #pde_terms_without_state = True
)

fv_solver.set_implementation(
  boundary_conditions  = """
    logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);
    Qoutside[0] = 0.0;
    Qoutside[1] = 0.0;
    Qoutside[2] = 0.0;
    logTraceOut("boundaryConditions(...)");
""",
  ncp                  = exahype2.solvers.fv.PDETerms.None_Implementation,
  flux                 = """
    logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);

    double rho = 2.7;
    double c = 6.0;
    double K_0 = rho * c * c;

    if (normal == 0) {
      F[0] = -K_0 * Q[1];
      F[1] = -1 / rho * Q[0];
      F[2] = 0.0;
    } else {
      F[0] = -K_0 * Q[2];
      F[1] = 0.0;
      F[2] = -1 / rho * Q[0];
    }
    logTraceOut("flux(...)");
""",
  source_term          = """
    logTraceInWith4Arguments("sourceTerm(...)", volumeCentre, volumeH, t, dt);

    tarch::la::Vector<Dimensions,double> centre = {5, 5};
    bool nearCentre = tarch::la::norm2(volumeCentre-centre)<0.1;
    if (nearCentre) {
      double sigma = 0.1149;
      double t0 = 0.7;
      double f = 0.0;
      double M0 = 1000.0;

      f = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * sigma * sigma));

      S[0] = f;
      S[1] = 0.0;
      S[2] = 0.0;
      logTraceOut("sourceTerm(...)");
    }
""",
  refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  eigenvalues          = """
    logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);
    return 6.0;
    logTraceOut("maxEigenvalue(...)");
""",
  initial_conditions   = """
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructred);
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
  logTraceOut("initialCondition(...)");
"""
)

project.add_solver( fv_solver )

#tracer_particles = project.add_tracer( name="Tracer", attribute_count=3 )
#project.add_action_set_to_initialisation( 
#  exahype2.tracer.InsertParticlesByCoordinates( 
#    particle_set=tracer_particles, coordinates=[[0.2,0.2]]))
#project.add_action_set_to_timestepping(
#  peano4.toolbox.particles.UpdateParallelState(
#    particle_set=tracer_particles
#  )
#)
#project.add_action_set_to_timestepping(
#  exahype2.tracer.FiniteVolumesTracing(
#    tracer_particles, 
#    thesolver, 
#    time_stepping_kernel="toolbox::particles::staticPosition",
#    project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
#  )
#)

#project.add_action_set_to_timestepping(
#  exahype2.tracer.DumpTrajectoryIntoDatabase(
#    particle_set=tracer_particles,
#    solver=thesolver,
#    filename="Tracer-" + solver_name,
#    data_delta_between_two_snapsots=1e16,
#    time_delta_between_two_snapsots=0.05
#  )
#)

if args.dim==2:
  offset = [0.0,0.0]
  size = [10.0,10.0]
else:
  offset = [0.0,0.0,0.0]
  size = [10.0,10.0,10.0]

project.set_global_simulation_parameters(
  dimensions = args.dim,
  offset = offset,
  size = size,
  min_end_time = 5.0,
  max_end_time = 5.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.1,
  periodic_BC           = [False, False, False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )

if args.debug:
  build_mode = peano4.output.CompileMode.Debug
else:
  build_mode = peano4.output.CompileMode.Release
project.set_Peano4_installation("../../../", build_mode )

peano4_project = project.generate_Peano4_project(verbose=args.verbose)
peano4_project.generate(throw_away_data_after_generation=True)
peano4_project.build(make_clean_first=True)

print("Done")
