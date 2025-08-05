# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

available_precisions = [
    "float", "double"
]

parser = exahype2.ArgumentParser("ExaHyPE 2 - Finite Volumes Testing Script")
parser.add_argument("-pr", "--precision", default="float", help="|".join(available_precisions))
args = parser.parse_args()

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = std::sin(x(0) * tarch::la::PI) * std::sin(x(1) * tarch::la::PI)
    #if DIMENSIONS == 3
      * std::sin(x(2) * tarch::la::PI)
    #endif
    ;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
"""

max_h = 1.1 / 3.0**args.min_depth
min_h = max_h * 3.0**(-args.amr_levels)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"v": args.dimensions},
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

fv_solver.set_solver_precisions(
  storage_precision = args.precision,
  compute_precision = args.precision
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="FVTest",
    directory=".",
    executable="FVTest",
)
project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[1.0, 1.0, 1.0][0:args.dimensions],
    offset=[0.0, 0.0, 0.0][0:args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
        args.periodic_boundary_conditions_z,
    ],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation("../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
