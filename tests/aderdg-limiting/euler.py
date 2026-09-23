# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def standard_arguments(
    end_time,
    degrees_of_freedom=6,
    min_grid_levels=2,
    max_grid_levels=3,
    limiter_modes=("active", "all-accepted", "all-limited"),
):
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument("-et", "--end-time", type=float, default=end_time)
    parser.add_argument("--degrees-of-freedom", type=int, default=degrees_of_freedom)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=min_grid_levels)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=max_grid_levels)
    parser.add_argument(
        "--limiter-solver",
        choices=("godunov", "muscl-hancock", "weno"),
        default="godunov",
    )
    parser.add_argument(
        "--limiter-mode",
        choices=limiter_modes,
        default="active",
    )
    arguments, _ = parser.parse_known_args()
    return arguments


def conserved_unknowns(dimensions):
    return {"rho": 1, "j": dimensions, "rhoE": 1}


def flux(gamma):
    return f"""
  constexpr double Gamma = {gamma};
  constexpr double DensityFloor = 1.0e-12;
  const double density = Q[Shortcuts::rho];
  const double energy = Q[Shortcuts::rhoE];
  // Transient predictor states at shocks may approach vacuum before limiter
  // rejection. The floor keeps the reciprocal finite without changing
  // admissible states.
  const double inverseDensity = 1.0 / std::max(density, DensityFloor);
  double momentumSquared = 0.0;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    momentumSquared +=
      Q[Shortcuts::j + dimension] * Q[Shortcuts::j + dimension];
  }}
  const double pressure =
    (Gamma - 1.0) * (energy - 0.5 * inverseDensity * momentumSquared);
  const double normalMomentum = Q[Shortcuts::j + normal];

  F[Shortcuts::rho] = normalMomentum;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    F[Shortcuts::j + dimension] =
      normalMomentum * Q[Shortcuts::j + dimension] * inverseDensity;
  }}
  F[Shortcuts::rhoE] =
    normalMomentum * (energy + pressure) * inverseDensity;
  F[Shortcuts::j + normal] += pressure;
"""


def max_eigenvalue(gamma):
    return f"""
  constexpr double Gamma = {gamma};
  constexpr double DensityFloor = 1.0e-12;
  const double density = Q[Shortcuts::rho];
  const double energy = Q[Shortcuts::rhoE];
  // Predictor and Riemann evaluations precede limiter rejection. Flooring the
  // density and taking the pressure magnitude keep finite inputs finite without
  // changing admissible states.
  const double inverseDensity = 1.0 / std::max(density, DensityFloor);
  double momentumSquared = 0.0;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    momentumSquared +=
      Q[Shortcuts::j + dimension] * Q[Shortcuts::j + dimension];
  }}
  const double pressure =
    (Gamma - 1.0) * (energy - 0.5 * inverseDensity * momentumSquared);
  const double soundSpeed =
    std::sqrt(std::abs(Gamma * pressure * inverseDensity));
  const double normalVelocity = Q[Shortcuts::j + normal] * inverseDensity;
  return std::abs(normalVelocity) + soundSpeed;
"""


def physical_admissibility(gamma, additional_state_condition=None):
    additional_state_check = ""
    if additional_state_condition is not None:
        additional_state_check = f"""
    if (not ({additional_state_condition})) {{
      return false;
    }}"""
    return f"""
  constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr double Gamma = {gamma};
  const auto* values = Q;
  for (int state = 0; state < numberOfStates; ++state, values += NumberOfData) {{
    for (int unknown = 0; unknown < NumberOfData; ++unknown) {{
      if (not std::isfinite(values[unknown])) {{
        return false;
      }}
    }}
    const double density = values[Shortcuts::rho];
    const double energy = values[Shortcuts::rhoE];
    // Densities near the vacuum threshold can cross it through last-bit
    // differences. Keeping them on the finite-volume scheme prevents a
    // one-sided status change from seeding asymmetry.
    if (density <= 1.0e-12 or energy <= 1.0e-12) {{
      return false;
    }}
    double momentumSquared = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
      momentumSquared +=
        values[Shortcuts::j + dimension] * values[Shortcuts::j + dimension];
    }}
    const double pressure =
      (Gamma - 1.0) * (energy - 0.5 * momentumSquared / density);
    if (not std::isfinite(pressure) or pressure <= 1.0e-12) {{
      return false;
    }}{additional_state_check}
  }}
  return true;
"""


def physical_admissibility_with_contact_sensor(
    gamma,
    light_density,
    heavy_density,
):
    return f"""
  constexpr int NumberOfData = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr double Gamma = {gamma};
  constexpr double LightDensity = {light_density};
  constexpr double HeavyDensity = {heavy_density};
  double minimumDensity = std::numeric_limits<double>::infinity();
  double maximumDensity = -std::numeric_limits<double>::infinity();
  const auto* values = Q;
  for (int state = 0; state < numberOfStates; ++state, values += NumberOfData) {{
    for (int unknown = 0; unknown < NumberOfData; ++unknown) {{
      if (not std::isfinite(values[unknown])) {{
        return false;
      }}
    }}
    const double density = values[Shortcuts::rho];
    const double energy = values[Shortcuts::rhoE];
    if (density <= 1.0e-12 or energy <= 1.0e-12) {{
      return false;
    }}
    minimumDensity = std::min(minimumDensity, density);
    maximumDensity = std::max(maximumDensity, density);
    double momentumSquared = 0.0;
    for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
      momentumSquared +=
        values[Shortcuts::j + dimension]
        * values[Shortcuts::j + dimension];
    }}
    const double pressure =
      (Gamma - 1.0) * (energy - 0.5 * momentumSquared / density);
    if (not std::isfinite(pressure) or pressure <= 1.0e-12) {{
      return false;
    }}
  }}
  const double contactDensity = 0.5 * (LightDensity + HeavyDensity);
  const bool crossesContact =
    minimumDensity < contactDensity and maximumDensity > contactDensity;
  const bool resolvesContact =
    maximumDensity - minimumDensity
    > 0.25 * (HeavyDensity - LightDensity);
  return not (crossesContact and resolvesContact);
"""


def dmp_observables(gamma):
    return f"""
  constexpr double Gamma = {gamma};
  constexpr {{{{COMPUTE_PRECISION}}}} DensityFloor = 1.0e-12;
  const {{{{COMPUTE_PRECISION}}}} density = Q[Shortcuts::rho];
  // The discrete maximum principle maps states before limiter rejection. The
  // floor keeps derived velocity and pressure finite without changing
  // admissible states.
  const {{{{COMPUTE_PRECISION}}}} inverseDensity =
    1.0 / std::max(density, DensityFloor);
  {{{{COMPUTE_PRECISION}}}} momentumSquared = 0.0;
  observables[0] = density;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {{
    const auto momentum = Q[Shortcuts::j + dimension];
    momentumSquared += momentum * momentum;
    observables[1 + dimension] = momentum * inverseDensity;
  }}
  observables[1 + DIMENSIONS] =
    (Gamma - 1.0)
    * (Q[Shortcuts::rhoE] - 0.5 * inverseDensity * momentumSquared);
"""


def subcell_scheme(limiter_solver_name):
    if limiter_solver_name == "muscl-hancock":
        return exahype2.fv.MusclHancock()
    if limiter_solver_name == "weno":
        return exahype2.fv.Weno()
    return exahype2.fv.Godunov()


def add_limiter(
    solver,
    limiter_solver_name,
    limiter_mode,
    dimensions,
    physical_admissibility_criterion,
    observable_mapping,
    differences_scaling,
):
    scheme = subcell_scheme(limiter_solver_name)
    if limiter_mode == "all-accepted":
        solver.set_pde_terms(physical_admissibility="return true;")
        solver.add_subcell_limiter(scheme)
    elif limiter_mode == "all-limited":
        solver.set_pde_terms(physical_admissibility="return false;")
        solver.add_subcell_limiter(scheme)
    else:
        solver.set_pde_terms(
            physical_admissibility=physical_admissibility_criterion,
            dmp_observables=observable_mapping,
        )
        solver.add_subcell_limiter(
            scheme,
            number_of_dmp_observables=dimensions + 2,
            dmp_relaxation_parameter=1.0e-4,
            dmp_differences_scaling=differences_scaling,
        )
