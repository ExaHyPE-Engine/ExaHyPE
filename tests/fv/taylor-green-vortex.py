# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

from PDE import max_eigenvalue, flux

initial_conditions = """
  constexpr double U0 = 1.0; // Initial amplitude of velocity
  constexpr double Pressure0 = 100 / GAMMA; // Reference pressure

  Q[Shortcuts::rho]      = 1.0;
  Q[Shortcuts::rhoU + 0] = U0 * std::sin(x(0)) * std::cos(x(1));
  Q[Shortcuts::rhoU + 1] = -U0 * std::cos(x(0)) * std::sin(x(1));
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2] = 0.0;
#endif

  const double pressure = Pressure0 + (Q[Shortcuts::rho] * U0) * (1.0 / 4.0) * (std::cos(2.0 * x(0)) + std::cos(2.0 * x(1)));
  // Total energy: internal energy + kinetic energy
  Q[Shortcuts::rhoE] = pressure / (GAMMA - 1.0) + 0.5  * (Q[Shortcuts::rhoU + 0] * Q[Shortcuts::rhoU + 0] + Q[Shortcuts::rhoU + 1] * Q[Shortcuts::rhoU + 1]);
"""

boundary_conditions = """
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

analytical_solution = """
  constexpr double Viscosity = 1.0;
  const double C = 100.0 / GAMMA;

  // Time decay factors for velocity and pressure
  const double expVelocityFactor = std::exp(-2.0 * Viscosity * t); // Time decay factor for velocity
  const double expPressureFactor = std::exp(-4.0 * Viscosity * t); // Time decay factor for pressure

  // Set the initial conditions for the Taylor-Green vortex
  solution[Shortcuts::rho] = 1.0; // Constant density

  // Velocity components with time decay
  solution[Shortcuts::rhoU + 0] = expVelocityFactor * std::sin(x(0)) * std::cos(x(1));
  solution[Shortcuts::rhoU + 1] = -expVelocityFactor * std::cos(x(0)) * std::sin(x(1));
#if DIMENSIONS == 3
  solution[Shortcuts::rhoU + 2] = 0.0;
#endif

  // Pressure with time decay and the given formula
  double pressure = expPressureFactor * (std::cos(2.0 * x(0)) + std::cos(2.0 * x(1)))*(1.0 / 4.0) + C;

  // Total energy: internal energy + kinetic energy
  solution[Shortcuts::rhoE] = pressure / (GAMMA - 1.0) + 0.5 * (Q[Shortcuts::rhoU + 0] * Q[Shortcuts::rhoU + 0] + Q[Shortcuts::rhoU + 1] * Q[Shortcuts::rhoU + 1]);
"""

parser = exahype2.ArgumentParser("ExaHyPE 2 - Finite Volumes Testing Script")
parser.set_defaults(
    min_depth=6,
    degrees_of_freedom=16,
    number_of_snapshots=0,
    periodic_boundary_conditions_x=True,
    periodic_boundary_conditions_y=True,
    periodic_boundary_conditions_z=True,
)
args = parser.parse_args()

size = [2 * 3.14159265359, 2 * 3.14159265359, 2 * 3.14159265359]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

fv_solver = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
    use_enclave_tasking=args.enclave_tasking,
    number_of_enclave_tasks=args.ntasks,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    limiter=exahype2.solvers.fv.musclhancock.Limiter.vanalbada,
)

fv_solver.set_plotter(peano4.plotter.VTUPatchAMRPlotter)

exahype2.solvers.fv.ErrorMeasurement(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error",
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="TaylorGreenVortex",
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
    size=size[0 : args.dimensions],
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

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.set_target_device(args.target_device)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
