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
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

#if DIMENSIONS == 3
  tarch::la::Vector<DIMENSIONS, double> centre0 = {2.0, 5.0, 5.0};
  tarch::la::Vector<DIMENSIONS, double> centre1 = {8.0, 5.0, 5.0};
  constexpr double maxRadius = 7.5;
#else
  tarch::la::Vector<DIMENSIONS, double> centre0 = {2.0, 5.0};
  tarch::la::Vector<DIMENSIONS, double> centre1 = {8.0, 5.0};
  constexpr double maxRadius = 5.0;
#endif

  constexpr double startRadius = MaxAdmissibleVolumeH;
  constexpr double growthRate  = 0.25; // radius = startRadius + growthRate*t
  constexpr double bandWidth   = 0.10;

  const double frontRadius = std::min(maxRadius, startRadius + growthRate * t);
  const double r0 = tarch::la::norm2(x - centre0);
  const double r1 = tarch::la::norm2(x - centre1);

  auto command0 = ::exahype2::RefinementCommand::Keep;
  auto command1 = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    if (r0 < startRadius) command0 = ::exahype2::RefinementCommand::Refine;
    if (r1 < startRadius) command1 = ::exahype2::RefinementCommand::Refine;
  } else {
    const double innerEraseRadius = std::max(0.0, frontRadius - bandWidth);

    if (r0 >= innerEraseRadius and r0 <= frontRadius) {
      command0 = ::exahype2::RefinementCommand::Refine;
    } else if (r0 < innerEraseRadius) {
      command0 = ::exahype2::RefinementCommand::Erase;
    }

    if (r1 >= innerEraseRadius and r1 <= frontRadius) {
      command1 = ::exahype2::RefinementCommand::Refine;
    } else if (r1 < innerEraseRadius) {
      command1 = ::exahype2::RefinementCommand::Erase;
    }
  }

  if (command0 == ::exahype2::RefinementCommand::Refine or command1 == ::exahype2::RefinementCommand::Refine) {
    result = ::exahype2::RefinementCommand::Refine;
  } else if (command0 == ::exahype2::RefinementCommand::Erase or command1 == ::exahype2::RefinementCommand::Erase) {
    result = ::exahype2::RefinementCommand::Erase;
  } else {
    result = ::exahype2::RefinementCommand::Keep;
  }

  return result;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=1,
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    normalised_time_step_size=0.01,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="""
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
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
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
