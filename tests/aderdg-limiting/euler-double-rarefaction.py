# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import euler

arguments = euler.standard_arguments(end_time=0.1)
order = arguments.degrees_of_freedom - 1
gamma = 1.4
domain_size = 27.0 / 26.0

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="EulerDoubleRarefaction",
    dimensions=2,
    size=[domain_size, domain_size, domain_size],
    offset=[0.0, -0.5 * domain_size, -0.5 * domain_size],
    periodic_bc=[False, True, True],
    min_end_time=arguments.end_time,
    max_end_time=arguments.end_time,
    min_grid_levels=arguments.min_grid_levels,
    max_grid_levels=arguments.max_grid_levels,
)
dimensions = project.dimensions
unknowns = euler.conserved_unknowns(dimensions)

initial_conditions = f"""
  constexpr double InterfacePosition = 0.5;
  constexpr double Gamma = {gamma};
  constexpr double Density = 1.0;
  constexpr double Pressure = 0.4;
  constexpr double VelocityMagnitude = 4.0;
  const double velocity =
    x[0] < InterfacePosition ? -VelocityMagnitude : VelocityMagnitude;

  Q[Shortcuts::rho] = Density;
  Q[Shortcuts::j] = Density * velocity;
  for (int dimension = 1; dimension < DIMENSIONS; ++dimension) {{
    Q[Shortcuts::j + dimension] = 0.0;
  }}
  Q[Shortcuts::rhoE] =
    Pressure / (Gamma - 1.0) + 0.5 * Density * velocity * velocity;
"""

boundary_conditions = """
  Qoutside[Shortcuts::rho] = Qinside[Shortcuts::rho];
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    Qoutside[Shortcuts::j + dimension] = Qinside[Shortcuts::j + dimension];
  }
  Qoutside[Shortcuts::rhoE] = Qinside[Shortcuts::rhoE];
"""

analytical_solution = f"""
  constexpr double Gamma = {gamma};
  constexpr double InterfacePosition = 0.5;
  constexpr double Density = 1.0;
  constexpr double Pressure = 0.4;
  constexpr double VelocityMagnitude = 4.0;
  const double soundSpeed = std::sqrt(Gamma * Pressure / Density);
  // The fan head runs at |u| + c. Its inner tail runs at
  // |u| - 2 c / (Gamma - 1), whose positive speed leaves vacuum between the
  // mirrored fans.
  const double head = VelocityMagnitude + soundSpeed;
  const double vacuumFront =
    VelocityMagnitude - 2.0 * soundSpeed / (Gamma - 1.0);
  const double side = x[0] < InterfacePosition ? -1.0 : 1.0;
  const double similarity =
    t > 0.0 ? std::abs(x[0] - InterfacePosition) / t : 2.0 * head;

  double density;
  double velocity;
  double pressure;
  if (similarity >= head) {{
    density = Density;
    velocity = VelocityMagnitude;
    pressure = Pressure;
  }} else if (similarity > vacuumFront) {{
    const double fanSoundSpeed =
      2.0 / (Gamma + 1.0)
      * (soundSpeed - 0.5 * (Gamma - 1.0) * (VelocityMagnitude - similarity));
    velocity =
      2.0 / (Gamma + 1.0)
      * (-soundSpeed + 0.5 * (Gamma - 1.0) * VelocityMagnitude + similarity);
    density =
      Density
      * std::pow(fanSoundSpeed / soundSpeed, 2.0 / (Gamma - 1.0));
    pressure =
      Pressure
      * std::pow(
        fanSoundSpeed / soundSpeed,
        2.0 * Gamma / (Gamma - 1.0)
      );
  }} else {{
    density = 0.0;
    velocity = 0.0;
    pressure = 0.0;
  }}

  solution[Shortcuts::rho] = density;
  solution[Shortcuts::j] = side * density * velocity;
  for (int dimension = 1; dimension < DIMENSIONS; ++dimension) {{
    solution[Shortcuts::j + dimension] = 0.0;
  }}
  solution[Shortcuts::rhoE] =
    pressure / (Gamma - 1.0) + 0.5 * density * velocity * velocity;
"""

refinement_criterion = """
  constexpr double InterfacePosition = 0.5;
  constexpr double RefinedHalfWidth = 0.35;
  const bool nearInterface =
    x[0] + 0.5 * h[0] >= InterfacePosition - RefinedHalfWidth
    and x[0] - 0.5 * h[0] <= InterfacePosition + RefinedHalfWidth;
  return nearInterface
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Keep;
"""

solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
)
solver.set_pde_terms(
    flux=euler.flux(gamma),
    max_eigenvalue=euler.max_eigenvalue(gamma),
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    analytical_solution=analytical_solution,
)

euler.add_limiter(
    solver,
    arguments.limiter_solver,
    arguments.limiter_mode,
    dimensions,
    euler.physical_admissibility(gamma),
    euler.dmp_observables(gamma),
    differences_scaling=1.0e-3,
)

solver.measure_error(file="Error_EulerDoubleRarefaction")

project.add_solver(solver)
project.generate_and_build()
