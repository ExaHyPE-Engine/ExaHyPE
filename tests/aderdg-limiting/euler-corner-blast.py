# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import euler

arguments = euler.standard_arguments(end_time=0.4)
order = arguments.degrees_of_freedom - 1
gamma = 1.4

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="EulerCornerBlast",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, True],
    min_end_time=arguments.end_time,
    max_end_time=arguments.end_time,
    min_grid_levels=arguments.min_grid_levels,
    max_grid_levels=arguments.max_grid_levels,
)
dimensions = project.dimensions
unknowns = euler.conserved_unknowns(dimensions)

initial_conditions = f"""
  constexpr double CornerExtent = 0.15;
  constexpr double Gamma = {gamma};
  constexpr double HighPressure = 5.0;
  constexpr double LowPressure = 1.0;
  const bool highState = x[0] < CornerExtent and x[1] < CornerExtent;

  Q[Shortcuts::rho] = highState ? 1.0 : 0.5;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    Q[Shortcuts::j + dimension] = 0.0;
  }}
  Q[Shortcuts::rhoE] =
    (highState ? HighPressure : LowPressure) / (Gamma - 1.0);
"""

boundary_conditions = """
  Qoutside[Shortcuts::rho] = Qinside[Shortcuts::rho];
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    Qoutside[Shortcuts::j + dimension] = Qinside[Shortcuts::j + dimension];
  }
  Qoutside[Shortcuts::rhoE] = Qinside[Shortcuts::rhoE];
  const bool isReflectiveWall = (normal == 0 and x[0] < DomainOffset[0] + h[0]) or (normal == 1 and x[1] < DomainOffset[1] + h[1]);
  if (isReflectiveWall) {
    Qoutside[Shortcuts::j + normal] = -Qinside[Shortcuts::j + normal];
  }
"""

refinement_criterion = """
  constexpr double RefinedRegionExtent = 0.6;
  const bool overlapsBlastRegion =
    x[0] - 0.5 * h[0] <= RefinedRegionExtent
    and x[1] - 0.5 * h[1] <= RefinedRegionExtent;
  return overlapsBlastRegion
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
