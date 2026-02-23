# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser(
    "ExaHyPE 2 - Finite Volumes Dynamic AMR Testing Script"
)
parser.set_defaults(
    min_depth=2,
    amr_levels=2,
    degrees_of_freedom=4,
    end_time=3.0,
)
args = parser.parse_args()

max_patch_size = 1.1 * 10.0 / (3.0**args.min_depth)
max_h = max_patch_size / args.degrees_of_freedom
min_h = 0.9 * max_h * 3.0 ** (-args.amr_levels)

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = 0.0;
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

#if DIMENSIONS == 2
  static constexpr double sourceRadius = 0.5;
  const tarch::la::Vector<DIMENSIONS, double> sourceCentre = {1.0, 1.0};
#else
  static constexpr double sourceRadius = 0.5;
  const tarch::la::Vector<DIMENSIONS, double> sourceCentre = {1.0, 1.0, 1.0};
#endif

  static constexpr double t0 = 0.7;
  if (tarch::la::greaterEquals(t, 0.0) and tarch::la::smallerEquals(t, t0)) {
    if (tarch::la::smallerEquals(tarch::la::norm2(x - sourceCentre), sourceRadius)) {
      result = ::exahype2::RefinementCommand::Refine;
    }
    return result;
  }

  const double pAbs = std::abs(Q[Shortcuts::p]);

  double u2 = 0.0;
  for (int i = 0; i < DIMENSIONS; i++) {
    const double ui = Q[Shortcuts::u + i];
    u2 += ui * ui;
  }
  const double uMag = std::sqrt(u2);

  const double indicator = std::max(pAbs, (RHO * WAVE_SPEED) * uMag);

  static constexpr double refineThreshold = 0.20;
  static constexpr double eraseThreshold  = 0.10;

  if (tarch::la::greaterEquals(indicator, refineThreshold)) {
    result = ::exahype2::RefinementCommand::Refine;
  } else if (tarch::la::smallerEquals(indicator, eraseThreshold)) {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"p": 1, "u": args.dimensions},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="""
  static constexpr double K0 = RHO * WAVE_SPEED * WAVE_SPEED;

  F[Shortcuts::p]     = K0 * Q[normal + 1];
  F[Shortcuts::u + 0] = 0.0;
  F[Shortcuts::u + 1] = 0.0;
#if DIMENSIONS == 3
  F[Shortcuts::u + 2] = 0.0;
#endif
  F[normal + 1]       = (1.0 / RHO) * Q[Shortcuts::p];

""",
    max_eigenvalue="return WAVE_SPEED;",
    source_term="""
  S[Shortcuts::p]     = 0.0;
  S[Shortcuts::u + 0] = 0.0;
  S[Shortcuts::u + 1] = 0.0;
#if DIMENSIONS == 3
  S[Shortcuts::u + 2] = 0.0;
#endif

#if DIMENSIONS == 2
  constexpr double sourceRadius = 0.10;
  const bool sourceLocation = tarch::la::norm2(x - tarch::la::Vector<DIMENSIONS, double>({1.0, 1.0})) <= sourceRadius;
#else
  constexpr double sourceRadius = 0.10;
  const bool sourceLocation = tarch::la::norm2(x - tarch::la::Vector<DIMENSIONS, double>({1.0, 1.0, 1.0})) <= sourceRadius;
#endif

  if (sourceLocation) {
    static constexpr double t0 = 0.7;
    static constexpr double M0 = 1000.0;
    const double force = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * SIGMA * SIGMA));
    S[Shortcuts::p] = force;
  }
""",
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)
project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[10.0, 10.0, 10.0][0 : args.dimensions],
    offset=[0.0, 0.0, 0.0][0 : args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        False,
        False,
        False,
    ],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.set_target_device(args.target_device)
project.output.makefile.add_CXX_flag("-DRHO=2.7")
project.output.makefile.add_CXX_flag("-DWAVE_SPEED=6.0")
project.output.makefile.add_CXX_flag("-DSIGMA=0.1149")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
