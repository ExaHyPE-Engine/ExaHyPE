# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser(
    "ExaHyPE 2 - Finite Volumes Dynamic AMR Testing Script"
)
parser.set_defaults(
    min_depth=2,
    amr_levels=4,
    degrees_of_freedom=4,
    end_time=1.5,
    number_of_snapshots=0,
)
args = parser.parse_args()

max_patch_size = 1.1 * 2.0 / (3.0**args.min_depth)
max_h = max_patch_size / args.degrees_of_freedom
min_h = 0.9 * max_h * 3.0 ** (-args.amr_levels)

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double radius = 0.1;

#if DIMENSIONS == 3
  tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0, 1.0};
#else
  tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0};
#endif

  if (tarch::la::norm2(x - centre) < radius) {
    Q[Shortcuts::rho] = 1.0;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  constexpr double radius = 0.1;

#if DIMENSIONS == 3
  const tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0, 1.0};
#else
  const tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0};
#endif

  if (tarch::la::equals(t, 0.0)) {
    return (tarch::la::smallerEquals(tarch::la::norm2(x - centre), radius))
      ? ::exahype2::RefinementCommand::Refine
      : ::exahype2::RefinementCommand::Keep;
  }

  const double rho = Q[Shortcuts::rho];

  return (tarch::la::greaterEquals(rho, 0.5) and tarch::la::smallerEquals(rho, 1.0))
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Erase;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions},
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
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
  F[normal] = Q[normal];
""",
    max_eigenvalue="return 1.0;",
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
    size=[2.0, 2.0, 2.0][0 : args.dimensions],
    offset=[0.0, 0.0, 0.0][0 : args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        True,
        False,
        False,
    ],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.set_target_device(args.target_device)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
