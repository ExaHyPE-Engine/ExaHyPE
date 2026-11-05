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

  Q.rho  = highState ? 1.0 : 0.5;
  Q.j    = 0.0;
  Q.rhoE = (highState ? HighPressure : LowPressure) / (Gamma - 1.0);
"""

boundary_conditions = f"""
  constexpr double Gamma = {gamma};
  constexpr double LowPressure = 1.0;
  const bool isReflectiveWall = (normal == 0 and x[0] < DomainOffset[0] + h[0]) or (normal == 1 and x[1] < DomainOffset[1] + h[1]);
  if (isReflectiveWall) {{
    Qoutside           = Qinside;
    Qoutside.j(normal) = -Qinside.j(normal);
  }} else {{
    Qoutside.rho  = 0.5;
    Qoutside.j    = 0.0;
    Qoutside.rhoE = LowPressure / (Gamma - 1.0);
  }}
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
