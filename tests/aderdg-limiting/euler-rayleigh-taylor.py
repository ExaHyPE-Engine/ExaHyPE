# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Compressible Euler Rayleigh-Taylor instability with a subcell limiter."""

import exahype2
import euler

arguments = euler.standard_arguments(end_time=2.0, degrees_of_freedom=4)
order = arguments.degrees_of_freedom - 1
gamma = 1.4

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="EulerRayleighTaylor",
    dimensions=2,
    size=[1.0, 2.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[True, False, True],
    min_end_time=arguments.end_time,
    max_end_time=arguments.end_time,
    min_grid_levels=arguments.min_grid_levels,
    max_grid_levels=arguments.max_grid_levels,
)
dimensions = project.dimensions
unknowns = euler.conserved_unknowns(dimensions)

initial_conditions = f"""
  constexpr double Gamma = {gamma};
  constexpr double RhoLight = 1.0;
  constexpr double RhoHeavy = 2.0;
  constexpr double InterfacePressure = 5.0;
  constexpr double Gravity = 1.0;
  constexpr int Vertical = 1;
  const double interfaceHeight = 1.0;
  const double horizontalPhase = 2.0 * tarch::la::PI * (x[0] - 0.5);
  double horizontalShape = std::cos(horizontalPhase);
  if constexpr (DIMENSIONS == 3) {{
    const double depthPhase = 2.0 * tarch::la::PI * (x[2] - 0.5);
    horizontalShape *= std::cos(depthPhase);
  }}
  const double perturbedInterface =
    interfaceHeight + 0.06 * horizontalShape;
  const double density =
    x[Vertical] < perturbedInterface ? RhoLight : RhoHeavy;
  const double distanceToInterface = x[Vertical] - interfaceHeight;
  const double pressure =
    x[Vertical] < interfaceHeight
      ? InterfacePressure - RhoLight * Gravity * distanceToInterface
      : InterfacePressure - RhoHeavy * Gravity * distanceToInterface;
  const double relativeInterfaceDistance = distanceToInterface / 2.0;
  const double verticalVelocity =
    0.03 * std::sqrt(2.0 * Gravity) * horizontalShape
    * std::exp(
      -100.0 * relativeInterfaceDistance * relativeInterfaceDistance
    );

  Q[Shortcuts::rho] = density;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    Q[Shortcuts::j + dimension] = 0.0;
  }}
  Q[Shortcuts::j + Vertical] = density * verticalVelocity;
  Q[Shortcuts::rhoE] =
    pressure / (Gamma - 1.0)
    + 0.5 * density * verticalVelocity * verticalVelocity;
"""

boundary_conditions = """
  constexpr int Vertical = 1;
  for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
    Qoutside[unknown] = Qinside[unknown];
  }
  if (normal == Vertical) {
    Qoutside[Shortcuts::j + Vertical] = -Qinside[Shortcuts::j + Vertical];
  }
"""

source_term = """
  constexpr double Gravity = 1.0;
  constexpr int Vertical = 1;
  for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
    S[unknown] = 0.0;
  }
  S[Shortcuts::j + Vertical] = -Gravity * Q[Shortcuts::rho];
  S[Shortcuts::rhoE] = -Gravity * Q[Shortcuts::j + Vertical];
"""

refinement_criterion = """
  constexpr double InterfaceHeight = 1.0;
  constexpr double RefinedHalfWidth = 0.35;
  constexpr int Vertical = 1;
  const bool nearInterface =
    x[Vertical] + 0.5 * h[Vertical] >= InterfaceHeight - RefinedHalfWidth
    and x[Vertical] - 0.5 * h[Vertical] <= InterfaceHeight + RefinedHalfWidth;
  return nearInterface
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Keep;
"""

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    unknowns=unknowns,
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)
solver.set_implementation(
    flux=euler.flux(gamma),
    max_eigenvalue=euler.max_eigenvalue(gamma),
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    source_term=source_term,
    refinement_criterion=refinement_criterion,
)

# The density contact stays admissible for the plain criterion, so the limiter
# would never fire and Gibbs oscillations would survive at the interface. The
# contact sensor rejects cells that straddle the contact with a resolved jump.
euler.add_limiter(
    solver,
    arguments.limiter_solver,
    arguments.limiter_mode,
    dimensions,
    euler.physical_admissibility_with_contact_sensor(
        gamma,
        light_density=1.0,
        heavy_density=2.0,
    ),
    euler.dmp_observables(gamma),
    differences_scaling=0.03,
)

project.add_solver(solver)
project.generate_and_build()
