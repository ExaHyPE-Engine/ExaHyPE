# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

"""
This is a particle tracing convergence test for linear plume rise.

This simulation implements a linear velocity field v = (0, ay) where particles experience
exponential vertical acceleration, designed to test the convergence properties of different
interpolation and integration schemes. In this one-way coupling setup, particles are advected
through the analytical velocity field, allowing for precise comparison between numerical and
analytical solutions.

The linear plume rise provides an ideal test case because:

- It has a known analytical solution: y(t) = y₀ * e^(at)
- It tests interpolation accuracy in a simple gradient field
- It validates integration scheme performance under exponential growth
- Particles maintain constant x-position while rising exponentially in y

Particles are initialised along a vertical line and their trajectories are compared
against the analytical solution to measure interpolation and integration errors.
"""


def generate_particles_on_vertical_line(
    file_path, num_particles=100, x_fixed=0.5, y_min=0.1, y_max=0.9
):
    """
    Generates a .dat file with particles uniformly spaced along a vertical line at x = x_fixed.

    Parameters:
        file_path (str): Path to save the .dat file.
        num_particles (int): Number of particles to generate.
        x_fixed (float): Fixed x-coordinate for all particles.
        y_min (float): Minimum y-coordinate.
        y_max (float): Maximum y-coordinate.
    """
    if num_particles < 2:
        y_values = [0.5]  # Fallback: single particle at center
    else:
        step = (y_max - y_min) / (num_particles - 1)
        y_values = [round(y_min + i * step, 4) for i in range(num_particles)]

    with open(file_path, "w") as file:
        for y in y_values:
            file.write(f"{x_fixed:.4f} {y:.4f}\n")

    print(
        f"Generated {num_particles} particles along vertical line at x = {x_fixed} and saved to {file_path}"
    )


parser = exahype2.ArgumentParser(
    "ExaHyPE 2 - Finite Volumes Particle Tracing Testing Script"
)

interpolation_methods = {
    "None": 0,  # No interpolation (use raw values)
    "Constant": 1,  # Piecewise constant interpolation (nearest neighbor)
    "Linear": 2,  # Piecewise linear interpolation (more accurate)
}
parser.add_argument(
    "-interp", "--interpolation", default="Linear", help="|".join(interpolation_methods)
)

integration_schemes = {
    "Static": 0,  # No integration (static particles)
    "Euler": 1,  # Explicit Euler method (simple but less accurate)
    "SemiEuler": 2,  # Semi-implicit Euler method (better stability)
    "Verlet": 3,  # Velocity Verlet algorithm (good for orbital mechanics)
    "Midpoint": 4,  # Midpoint method (2nd order Runge-Kutta)
    "RK3": 5,  # 3rd order Runge-Kutta method (high accuracy)
    "RK4": 6,  # 4th order Runge-Kutta method (higher accuracy)
}
parser.add_argument(
    "-integ", "--integration", default="SemiEuler", help="|".join(integration_schemes)
)

gravity_models = {
    "None": 0,  # No gravity (straight-line motion)
    "Central": 1,  # Central gravitational force (1/r²) from a fixed point
    "Custom": 2,  # User-defined gravitational field
}
parser.add_argument(
    "-grav", "--gravity", default="Central", help="|".join(gravity_models)
)

parser.set_defaults(
    min_depth=3,
    degrees_of_freedom=64,
    end_time=1.0,
    number_of_snapshots=20,
)

args = parser.parse_args()

size = [3.0, 3.0]
offset = [0.0, 0.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

initial_conditions = """
  Q[Shortcuts::rho]       = 1.0;
  Q[Shortcuts::rhoU + 0]  = 0.0;        // rho * u_x
  Q[Shortcuts::rhoU + 1]  = 0.5 * x(1); // rho * u_y
  Q[Shortcuts::rhoE]      = 1.0;        // Simple energy (rho*e)
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

flux = """
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
"""

max_eigenvalue = """
  return 1.0;
"""

analytical_solution = """
  // Analytical solution for linear plume rise with exponential velocity growth in y-direction
  constexpr double a = 0.5; // Velocity gradient coefficient

  // Initial particle position (x0, y0)
  const double x0 = particle->getData(1); // x doesn't change
  const double y0 = particle->getData(2); // Used to reconstruct y(t) analytically

  // Time-dependent solution
  solution[0] = x0;                       // x(t) = x0 (no x motion)
  solution[1] = y0 * exp(a * t);          // y(t) = y0 * e^(a t)

  // Fluid velocity field at particle position
  solution[3] = 0.0;                      // fluid_u (x-direction)
  solution[4] = a * solution[1];          // fluid_v = a * y

  // Particle velocity (matches fluid)
  solution[5] = 0.0;                      // u = dx/dt
  solution[6] = a * solution[1];          // v = dy/dt

  // Particle acceleration
  solution[7] = 0.0;                      // a_x
  solution[8] = a * a * solution[1];      // a_y = d^2y/dt^2 = a^2 * y(t)

  solution[2] = 1.0; // rho
  solution[9] = 1.0; // mass
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": 2, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    normalised_time_step_size=0.01,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

tracer_attributes = {
    "unknowns": {
        "rho": 1,
        "init_x": 1,
        "init_y": 1,  # Interpolated fluid velocity
        "u": 1,
        "v": 1,  # Particle's own velocity
        "a_x": 1,
        "a_y": 1,
        "d": 1,
    }
}

tracer_particles = fv_solver.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

generate_particles_on_vertical_line(
    "InitTracers.dat", num_particles=1000, x_fixed=1.5, y_min=0.1, y_max=0.7
)
init_tracers = exahype2.tracer.InsertParticlesFromFile(
    particle_set=tracer_particles,
    filename="InitTracers.dat",
    scale_factor=1.0,
    save_initialpos=True,
)

init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

tracing_action_set = exahype2.tracer.FiniteVolumesTracingWithForce(
    tracer_particles,
    fv_solver,
    project,
    integration_scheme=integration_schemes[args.integration],
    interpolation_scheme=interpolation_methods[args.interpolation],
)

tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)

project.add_action_set_to_timestepping(tracing_action_set)

from pathlib import Path

error_measurement = exahype2.solvers.fv.ErrorMeasurementParticles(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name=args.output + "/Error_" + Path(__file__).name,
    particle_set=tracer_particles,
    descend_invocation_order=65536,  # After all other action sets
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation(
    "../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
