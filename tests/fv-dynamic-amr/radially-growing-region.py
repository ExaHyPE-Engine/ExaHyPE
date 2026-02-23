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
  tarch::la::Vector<DIMENSIONS, double> centre0 = {5.0, 5.0, 5.0};
  constexpr double maxRadius = 7.5;
#else
  tarch::la::Vector<DIMENSIONS, double> centre0 = {5.0, 5.0};
  constexpr double maxRadius = 5.0;
#endif

  constexpr double startRadius = MaxAdmissibleVolumeH;
  constexpr double growthRate  = 0.25; // radius = startRadius + growthRate*t
  constexpr double bandWidth   = 0.05;

  const double frontRadius = std::min(maxRadius, startRadius + growthRate * t);
  const double r = tarch::la::norm2(x - centre0);

  if (tarch::la::equals(t, 0.0)) {
    if (r < startRadius) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  } else {
    const double innerEraseRadius = std::max(0.0, frontRadius - bandWidth);
    if (r >= innerEraseRadius and r <= frontRadius) {
      result = ::exahype2::RefinementCommand::Refine;
    } else if (r < innerEraseRadius) {
      result = ::exahype2::RefinementCommand::Erase;
    }
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
