# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

project = exahype2.Project(
    ["tests", "exahype2", "particles"], ".", executable="FV-Particles"
)

# Configuration parameters
dimensions_ = 2
end_time_ = 0.5
width_ = [1.0, 1.0, 1.0]
offset_ = [0.0, 0.0, 0.0]
pbc_X = True
pbc_Y = False
pbc_Z = False
min_depth_ = 3
amr_levels_ = 0
patch_size_ = 64
number_of_snapshots_ = 20
time_step_relaxation_ = 0.5
output_dir_ = "solutions"
peano_dir_ = "../../.."
# Calculate max and min volume sizes
max_h_ = 1.1 * min(width_[0:dimensions_]) / (3.0**min_depth_)
min_h_ = max_h_ * 3.0**amr_levels_
time_in_between_plots = end_time_ / number_of_snapshots_
# Initial conditions
initial_conditions_ = """
for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = std::sin(x(0) * tarch::la::PI) * std::sin(x(1) * tarch::la::PI)
    #if Dimensions == 3
      * std::sin(x(2) * tarch::la::PI)
    #endif
    ;
  }"""

# Boundary conditions
boundary_conditions_ = """
for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }"""

# Flux function
flux_ = """
  F[0] = 0.0;
  F[1] = 0.0;
  #if Dimensions == 3
  F[2] = 0.0;
  #endif
  F[normal] = Q[normal];"""

# Refinement criteria
refinement_criteria_ = """
  auto result = ::exahype2::RefinementCommand::Keep;
  if (x(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }
  return result;"""

# Eigenvalues function
eigenvalues_ = """
return 1.0;
"""

# Create solver
fvSolver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="Advection",
    patch_size=patch_size_,
    min_volume_h=min_h_,
    max_volume_h=max_h_,
    time_step_relaxation=time_step_relaxation_,
    unknowns=dimensions_,
    auxiliary_variables=0,
    initial_conditions=initial_conditions_,
    boundary_conditions=boundary_conditions_,
    max_eigenvalue=eigenvalues_,
    flux=flux_,
)
fvSolver.switch_storage_scheme(
    exahype2.solvers.Storage["CallStack"], exahype2.solvers.Storage["CallStack"]
)

project.set_global_simulation_parameters(
    dimensions=dimensions_,
    size=width_[0:dimensions_],
    offset=offset_[0:dimensions_],
    min_end_time=end_time_,
    max_end_time=end_time_,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        pbc_X,
        pbc_Y,
        pbc_Z,
    ],
)

project.add_solver(fvSolver)
project.set_output_path("solutions")

build_mode = peano4.output.CompileMode.Release
project.set_Peano4_installation(peano_dir_, build_mode)

# Set up Tracer particles
particle_attributes_ = {
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
    if dimensions_ == 3
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
    name="Tracers", particle_attributes=particle_attributes_
)

init_action_set = exahype2.tracer.InsertParticlesFromFile(
    particle_set=tracer_particles,
    filename="particles_2d.dat",
    scale_factor=1.0,
)
init_action_set.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_action_set)

tracing_action_set = exahype2.tracer.FiniteVolumesTracing(
    tracer_particles,
    fvSolver,
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
""",)

tracing_action_set.descend_invocation_order = (
    fvSolver._action_set_update_cell.descend_invocation_order + 1
)
project.add_action_set_to_timestepping(tracing_action_set)
project.add_action_set_to_initialisation(tracing_action_set)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
    "new ::exahype2::LoadBalancingConfiguration()",
)

project = project.generate_Peano4_project("False")
project.build(make_clean_first=True)

print(fvSolver)
print(project)
print(tracer_particles)
print(tracing_action_set)
