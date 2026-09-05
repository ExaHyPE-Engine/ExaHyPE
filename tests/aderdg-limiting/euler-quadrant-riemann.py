# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import euler

arguments = euler.standard_arguments(end_time=2.0)
order = arguments.degrees_of_freedom - 1
gamma = 1.4

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="EulerQuadrantRiemann",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_bc=[False, False, True],
    min_end_time=arguments.end_time,
    max_end_time=arguments.end_time,
    min_grid_levels=arguments.min_grid_levels,
    max_grid_levels=arguments.max_grid_levels,
)
dimensions = project.dimensions
unknowns = euler.conserved_unknowns(dimensions)

initial_conditions = f"""
  constexpr double ShockPosition = -0.111;
  constexpr double Gamma = {gamma};
  const bool lowState =
    x[0] < ShockPosition and x[1] < ShockPosition;
  const double pressure = lowState ? 0.1 : 1.0;

  Q[Shortcuts::rho] = lowState ? 0.125 : 1.0;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    Q[Shortcuts::j + dimension] = 0.0;
  }}
  Q[Shortcuts::rhoE] = pressure / (Gamma - 1.0);
"""

boundary_conditions = f"""
  constexpr double ShockPosition = -0.111;
  constexpr double Gamma = {gamma};
  const bool lowState =
    x[0] < ShockPosition and x[1] < ShockPosition;
  const double pressure = lowState ? 0.1 : 1.0;

  Qoutside[Shortcuts::rho] = lowState ? 0.125 : 1.0;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    Qoutside[Shortcuts::j + dimension] = 0.0;
  }}
  Qoutside[Shortcuts::rhoE] = pressure / (Gamma - 1.0);
"""

refinement_criterion = """
  constexpr double ShockPosition = -0.111;
  constexpr double RefinedHalfWidth = 0.5;
  const bool nearVerticalInterface =
    x[0] + 0.5 * h[0] >= ShockPosition - RefinedHalfWidth
    and x[0] - 0.5 * h[0] <= ShockPosition + RefinedHalfWidth;
  const bool nearHorizontalInterface =
    x[1] + 0.5 * h[1] >= ShockPosition - RefinedHalfWidth
    and x[1] - 0.5 * h[1] <= ShockPosition + RefinedHalfWidth;
  return nearVerticalInterface and nearHorizontalInterface
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
    refinement_criterion=refinement_criterion,
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

project.add_solver(solver)
project.generate_and_build()
