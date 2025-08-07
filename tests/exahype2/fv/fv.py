# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

available_precisions = ["float", "double"]

parser = exahype2.ArgumentParser("ExaHyPE 2 - Finite Volumes Testing Script")
parser.add_argument(
    "-pr", "--precision", default="double", help="|".join(available_precisions)
)
parser.set_defaults(
    min_depth=6,
    degrees_of_freedom=16,
    periodic_boundary_conditions_x=False,  # Throws assertion in the TracersSet when set to True
    periodic_boundary_conditions_y=False,
    periodic_boundary_conditions_z=False,
)
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
min_h = max_h * 3.0 ** (-args.amr_levels)

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
    storage_precision=args.precision, compute_precision=args.precision
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="FVTest",
    directory=".",
    executable="FVTest",
)
project.add_solver(fv_solver)

particle_attributes = {
    "unknowns": {
        "rho": 1,
        "fluid_u": 1,
        "fluid_v": 1,
        "fluid_w": 1,  # Interpolated fluid velocity
        "u": 1,
        "v": 1,
        "w": 1,  # Particle's own velocity
        "a_x": 1,
        "a_y": 1,
        "a_z": 1,
        "d": 1,
    }
    if args.dimensions == 3
    else {
        "rho": 1,
        "fluid_u": 1,
        "fluid_v": 1,  # Interpolated fluid velocity
        "u": 1,
        "v": 1,  # Particle's own velocity
        "a_x": 1,
        "a_y": 1,
        "d": 1,
    }
}

tracer_particles = project.add_tracer(
    name="Tracers", particle_attributes=particle_attributes
)

init_action_set = exahype2.tracer.InsertParticlesFromFile(
    particle_set=tracer_particles,
    filename="ICParticles.dat",
    scale_factor=1.0,
)

init_action_set.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_action_set)

tracing_action_set = exahype2.tracer.FiniteVolumesTracing(
    tracer_particles,
    fv_solver,
    project_on_tracer_properties_kernel="::exahype2::fv::updateParticleState<0, 1, 2>",
    projection_kernel_arguments="""
  *p,
  fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
  marker,
  {{PATCH_SIZE}},
  {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
  fineGridCell{{SOLVER_NAME}}Q.value,
  {1,1,1},
  {0,0,0},
  {1,0,0}
""",
)

tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)
project.add_action_set_to_initialisation(tracing_action_set)
#project.add_action_set_to_timestepping(tracing_action_set)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[1.0, 1.0, 1.0][0 : args.dimensions],
    offset=[0.0, 0.0, 0.0][0 : args.dimensions],
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
project.set_Peano4_installation(
    "../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
