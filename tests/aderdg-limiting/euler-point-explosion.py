# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import euler

arguments = euler.standard_arguments(end_time=0.1, degrees_of_freedom=4)
order = arguments.degrees_of_freedom - 1
gamma = 1.4

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="EulerPointExplosion",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=arguments.end_time,
    max_end_time=arguments.end_time,
    min_grid_levels=arguments.min_grid_levels,
    max_grid_levels=arguments.max_grid_levels,
)
dimensions = project.dimensions
unknowns = euler.conserved_unknowns(dimensions)

initial_conditions = f"""
  constexpr double Gamma = {gamma};
  constexpr double Radius = 0.15;
  constexpr double HighPressure = 5.0;
  constexpr double LowPressure = 1.0;
  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  const bool highState = tarch::la::norm2(x - centre) <= Radius;

  Q.rho  = 1.0;
  Q.j    = 0.0;
  Q.rhoE = (highState ? HighPressure : LowPressure) / (Gamma - 1.0);
"""

boundary_conditions = """
  Qoutside           = Qinside;
  Qoutside.j(normal) = -Qinside.j(normal);
"""

refinement_criterion = f"""
  return ::exahype2::refineOnDetail(
    Q,
    Quadrature<double>::getQuadraturePoints1D(),
    2.0e-2,
    ::exahype2::DefaultCoarsenSafetyFactor,
    [](const auto& state) {{ return state.rho; }},
    [](const auto& state) {{
      constexpr double Gamma = {gamma};
      constexpr double DensityFloor = 1.0e-12;
      const double density = std::max(state.rho, DensityFloor);
      double momentumSquared = 0.0;
      for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
        momentumSquared += state.j(dimension) * state.j(dimension);
      }}
      return std::max(
        (Gamma - 1.0)
          * (state.rhoE - 0.5 * momentumSquared / density),
        DensityFloor
      );
    }}
  );
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
