# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import euler

arguments = euler.standard_arguments(
    end_time=10.0,
    min_grid_levels=3,
    max_grid_levels=4,
    limiter_modes=("fv", "active", "all-accepted", "all-limited"),
)
order = arguments.degrees_of_freedom - 1
limiter_patch_size = 2 * order + 1
gamma = 1.4
auxiliary_variables = {"volumeFraction": 1}

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="EulerAirfoil",
    dimensions=2,
    size=[120.0, 120.0, 120.0],
    offset=[-10.0, -60.0, -60.0],
    periodic_bc=[False, False, True],
    min_end_time=arguments.end_time,
    max_end_time=arguments.end_time,
    min_grid_levels=arguments.min_grid_levels,
    max_grid_levels=arguments.max_grid_levels,
)
dimensions = project.dimensions
unknowns = euler.conserved_unknowns(dimensions)

initial_conditions = """
  constexpr double Chord = 100.0;
  constexpr double Thickness = 0.12;
  constexpr int NumberOfSamples = 50;

  Q[Shortcuts::rho] = 1.0;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    Q[Shortcuts::j + dimension] = 0.0;
  }
  Q[Shortcuts::j] = 0.1;
  Q[Shortcuts::rhoE] = 1.0;

  const double lowerCellBoundary = x[1] - 0.5 * h[1];
  const double upperCellBoundary = x[1] + 0.5 * h[1];
  const double sampleWidth = h[0] / NumberOfSamples;
  double solidFraction = 0.0;
  for (int sample = 0; sample < NumberOfSamples; ++sample) {
    const double samplePosition =
      x[0] - 0.5 * h[0] + (sample + 0.5) * sampleWidth;
    double halfThickness = 0.0;
    if (samplePosition >= 0.0 and samplePosition <= Chord) {
      const double chordPosition = samplePosition / Chord;
      halfThickness =
        5.0 * Thickness * Chord
        * (
          0.2969 * std::sqrt(chordPosition)
          - 0.1260 * chordPosition
          - 0.3516 * chordPosition * chordPosition
          + 0.2843 * chordPosition * chordPosition * chordPosition
          - 0.1015 * chordPosition * chordPosition * chordPosition
            * chordPosition
        );
    }
    const double overlap = std::max(
      0.0,
      std::min(upperCellBoundary, halfThickness)
        - std::max(lowerCellBoundary, -halfThickness)
    );
    solidFraction += overlap / h[1] / NumberOfSamples;
  }
  Q[Shortcuts::volumeFraction] =
    std::max(0.0, std::min(1.0, 1.0 - solidFraction));
"""

boundary_conditions = """
  if (normal == 0 and tarch::la::equals(x[0], DomainOffset[0])) {
    Qoutside[Shortcuts::rho] = 1.0;
    for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
      Qoutside[Shortcuts::j + dimension] = 0.0;
    }
    Qoutside[Shortcuts::j] = 0.1;
    Qoutside[Shortcuts::rhoE] = 1.0;
  } else {
    for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
      Qoutside[unknown] = Qinside[unknown];
    }
  }
  Qoutside[Shortcuts::volumeFraction] = 1.0;
"""

refinement_criterion = """
  constexpr double Chord = 100.0;
  constexpr double RefinementHalfWidth = 10.0;
  const bool overlapsChord =
    x[0] + 0.5 * h[0] >= 0.0 and x[0] - 0.5 * h[0] <= Chord;
  const bool overlapsAirfoilBand =
    x[1] + 0.5 * h[1] >= -RefinementHalfWidth
    and x[1] - 0.5 * h[1] <= RefinementHalfWidth;
  return overlapsChord and overlapsAirfoilBand
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Keep;
"""

flux = euler.flux(gamma)
max_eigenvalue = euler.max_eigenvalue(gamma)

limiter_riemann_solver = """
  const double maximumWaveSpeed =
    std::max(
      maxEigenvalue(QL, x, h, t, dt, normal),
      maxEigenvalue(QR, x, h, t, dt, normal)
    );

  flux(QL, x, h, t, dt, normal, FL);
  flux(QR, x, h, t, dt, normal, FR);
  const auto normalMomentumFluxLeft = FL[Shortcuts::j + normal];
  const auto normalMomentumFluxRight = FR[Shortcuts::j + normal];

  const double volumeFractionLeft  = QL[Shortcuts::volumeFraction];
  const double volumeFractionRight = QR[Shortcuts::volumeFraction];
  // The opposing cell's fluid fraction transmits the Rusanov flux. Its solid
  // remainder acts as a wall and reflects the interior normal-momentum flux.
  for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
    const double rusanovFlux =
      0.5
      * (
        FL[unknown] + FR[unknown]
        + maximumWaveSpeed * (QL[unknown] - QR[unknown])
      );
    FL[unknown] = volumeFractionRight * rusanovFlux;
    FR[unknown] = volumeFractionLeft * rusanovFlux;
  }
  FL[Shortcuts::j + normal] +=
    (1.0 - volumeFractionRight) * normalMomentumFluxLeft;
  FR[Shortcuts::j + normal] +=
    (1.0 - volumeFractionLeft) * normalMomentumFluxRight;
  return maximumWaveSpeed;
"""

static_geometry_admissibility = euler.physical_admissibility(
    gamma,
    "numberOfStates == 1 or values[Shortcuts::volumeFraction] >= 1.0 - 1.0e-12",
)
if arguments.limiter_solver == "muscl-hancock":
    limiter_solver_class = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep
    limiter_scheme = exahype2.solvers.fv.musclhancock.subcell_scheme(
        riemann_solver=limiter_riemann_solver
    )
else:
    limiter_solver_class = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep
    limiter_scheme = exahype2.solvers.fv.godunov.subcell_scheme(
        riemann_solver=limiter_riemann_solver
    )

if arguments.limiter_mode == "fv":
    limiter_solver = limiter_solver_class(
        name="FVSolver",
        patch_size=limiter_patch_size,
        unknowns=unknowns,
        auxiliary_variables=auxiliary_variables,
        time_step_relaxation=0.5,
    )
    limiter_solver.set_implementation(
        flux=flux,
        max_eigenvalue=max_eigenvalue,
        riemann_solver=limiter_riemann_solver,
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
        refinement_criterion=refinement_criterion,
    )
    project.add_solver(limiter_solver)
else:
    solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
        name="ADERDGSolver",
        order=order,
        unknowns=unknowns,
        auxiliary_variables=auxiliary_variables,
        time_step_relaxation=0.5,
    )
    solver.set_implementation(
        flux=flux,
        max_eigenvalue=max_eigenvalue,
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
        refinement_criterion=refinement_criterion,
    )
    solver.add_kernel_optimisations(
        polynomials=exahype2.solvers.aderdg.ADERDG.Polynomials.Gauss_Lobatto
    )
    if arguments.limiter_mode == "all-accepted":
        admissibility_criterion = "return true;"
    elif arguments.limiter_mode == "all-limited":
        admissibility_criterion = euler.physical_admissibility(
            gamma, "numberOfStates == 1"
        )
    else:
        admissibility_criterion = static_geometry_admissibility
    solver.add_subcell_limiter(
        limiter_solver=limiter_scheme,
        physical_admissibility_criterion=admissibility_criterion,
    )
    project.add_solver(solver)

project.generate_and_build()
