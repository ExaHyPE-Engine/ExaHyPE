# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

from PDE import max_eigenvalue, flux, source_term
from FWave import fWave


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=6)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  const auto scaleHeight =
    GAS_CONSTANT * TEMPERATURE / GRAVITY / MEAN_MOLECULAR_MASS;
  const auto exponentialDistribution =
    tarch::la::pow(tarch::la::E, -x(1) / scaleHeight);

  Q[Shortcuts::rho]       = GROUND_DENSITY * exponentialDistribution;
  Q[Shortcuts::rhoU + 0]  = 0.0;
  Q[Shortcuts::rhoU + 1]  = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2]  = 0.0;
#endif

  const auto pressure =
    Q[Shortcuts::rho] * GAS_CONSTANT * TEMPERATURE / MEAN_MOLECULAR_MASS;
  Q[Shortcuts::rhoE] = pressure / (GAMMA - 1.0);
"""

boundary_conditions = """
  if (normal == 1) {
    const bool isUpperBoundary = x(1) > DomainOffset(1) + 0.5 * DomainSize(1);
    const auto verticalDisplacement = isUpperBoundary ? h(1) : -h(1);
    const auto scaleHeight =
      GAS_CONSTANT * TEMPERATURE / GRAVITY / MEAN_MOLECULAR_MASS;
    const auto exponentialDistribution = tarch::la::pow(
      tarch::la::E,
      -verticalDisplacement / scaleHeight
    );

    const auto densityOutside =
      Qinside[Shortcuts::rho] * exponentialDistribution;
    const auto pressureOutside =
      densityOutside * GAS_CONSTANT * TEMPERATURE / MEAN_MOLECULAR_MASS;

    Qoutside[Shortcuts::rho] = densityOutside;
    const auto horizontalVelocity =
      Qinside[Shortcuts::rhoU + 0] / Qinside[Shortcuts::rho];
    const auto verticalVelocity =
      Qinside[Shortcuts::rhoU + 1] / Qinside[Shortcuts::rho];
#if DIMENSIONS == 3
    const auto depthVelocity =
      Qinside[Shortcuts::rhoU + 2] / Qinside[Shortcuts::rho];
    const auto velocitySquared =
      horizontalVelocity * horizontalVelocity
      + verticalVelocity * verticalVelocity
      + depthVelocity * depthVelocity;
#else
    const auto velocitySquared =
      horizontalVelocity * horizontalVelocity
      + verticalVelocity * verticalVelocity;
#endif

    Qoutside[Shortcuts::rhoE] =
      pressureOutside / (GAMMA - 1.0)
      + 0.5 * densityOutside * velocitySquared;

    Qoutside[Shortcuts::rhoU + 0] = horizontalVelocity * densityOutside;
#if DIMENSIONS == 3
    Qoutside[Shortcuts::rhoU + 2] = depthVelocity * densityOutside;
#endif

    if (isUpperBoundary) {
      Qoutside[Shortcuts::rhoU + 1] = verticalVelocity * densityOutside;
    } else {
      // The lower wall prevents atmosphere leaving through the surface.
      Qoutside[Shortcuts::rhoU + 1] = -verticalVelocity * densityOutside;
    }
  } else {
    for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
      Qoutside[unknown] = Qinside[unknown];
    }
  }
"""

compute_usq = """
  auto computeVelocitySquared = [](
    const {{COMPUTE_PRECISION}}* const NOALIAS state,
    {{COMPUTE_PRECISION}}& velocitySquared
  ) {
    constexpr int ShortcutRho = 0;
    constexpr int ShortcutRhoU = 1;

    const auto horizontalVelocity = state[ShortcutRhoU + 0] / state[ShortcutRho];
    const auto verticalVelocity = state[ShortcutRhoU + 1] / state[ShortcutRho];
#if DIMENSIONS == 3
    const auto depthVelocity = state[ShortcutRhoU + 2] / state[ShortcutRho];
    velocitySquared =
      horizontalVelocity * horizontalVelocity
      + verticalVelocity * verticalVelocity
      + depthVelocity * depthVelocity;
#else
    velocitySquared =
      horizontalVelocity * horizontalVelocity
      + verticalVelocity * verticalVelocity;
#endif
  };
"""

calculate_pressure = (
    """
  auto calculatePressure = [](
    const {{COMPUTE_PRECISION}}* const NOALIAS state,
    {{COMPUTE_PRECISION}}& pressure,
    {{COMPUTE_PRECISION}}& velocitySquared
  ) {
    constexpr int ShortcutRho = 0;
    constexpr int ShortcutRhoE = DIMENSIONS + 1;

    """
    + compute_usq
    + """

    computeVelocitySquared(state, velocitySquared);
    pressure =
      (GAMMA - 1.0)
      * (
        state[ShortcutRhoE]
        - 0.5 * state[ShortcutRho] * velocitySquared
      );
  };
"""
)

calculate_pressure_perturbations = (
    """
  auto pressurePerturbations = [](
    const {{COMPUTE_PRECISION}}* const NOALIAS leftState,
    const {{COMPUTE_PRECISION}}* const NOALIAS state,
    const {{COMPUTE_PRECISION}}* const NOALIAS rightState,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    const int normal,
    {{COMPUTE_PRECISION}}& pressurePerturbationLeft,
    {{COMPUTE_PRECISION}}& pressurePerturbationRight
  ) {
    // Equations 25–26 of Käppeli and Mishra define perturbations against
    // the local hydrostatic pressure reconstruction.
    const auto densityLeft = leftState[0];
    const auto density = state[0];
    const auto densityRight = rightState[0];

    """
    + calculate_pressure
    + """

    {{COMPUTE_PRECISION}} pressure;
    {{COMPUTE_PRECISION}} velocitySquared;
    calculatePressure(state, pressure, velocitySquared);

    {{COMPUTE_PRECISION}} pressureLeft;
    {{COMPUTE_PRECISION}} velocitySquaredLeft;
    calculatePressure(leftState, pressureLeft, velocitySquaredLeft);

    {{COMPUTE_PRECISION}} pressureRight;
    {{COMPUTE_PRECISION}} velocitySquaredRight;
    calculatePressure(rightState, pressureRight, velocitySquaredRight);

    const auto reconstructedPressureLeft =
      pressure
      + 0.5 * (densityLeft + density) * -GRAVITY * volumeSize(normal);
    const auto reconstructedPressureRight =
      pressure
      - 0.5 * (densityRight + density) * -GRAVITY * volumeSize(normal);

    pressurePerturbationLeft = pressureLeft - reconstructedPressureLeft;
    pressurePerturbationRight = pressureRight - reconstructedPressureRight;
  };
"""
)

limiter = calculate_pressure_perturbations + """
  {{COMPUTE_PRECISION}} rightDifference;
  {{COMPUTE_PRECISION}} leftDifference;
  if (unknown == 3 and normal == 1) {
    pressurePerturbations(
      QL,
      Q,
      QR,
      h,
      normal,
      leftDifference,
      rightDifference
    );
  } else {
    rightDifference = QR[unknown] - Q[unknown];
    leftDifference = Q[unknown] - QL[unknown];
  }

  if (rightDifference * leftDifference <= 0.0) {
    return 0.0;
  }
  return tarch::la::smaller(
    tarch::la::abs(rightDifference),
    tarch::la::abs(leftDifference)
  )
    ? rightDifference
    : leftDifference;
"""

# The Riemann solver reads the hydrostatically reconstructed face states.
reconstruction = calculate_pressure + compute_usq + """
  constexpr int ShortcutRho = 0;
  constexpr int ShortcutE = DIMENSIONS + 1;

  if (unknown == ShortcutE and normal == 1) {
    {{COMPUTE_PRECISION}} pressure;
    {{COMPUTE_PRECISION}} velocitySquared;
    calculatePressure(Q, pressure, velocitySquared);

    const {{COMPUTE_PRECISION}} density = Q[ShortcutRho];

    // Eq. 16 Käppeli and Mishra: The first-order hydrostatic pressure at either
    // boundary. The second-order pressure-perturbation slope is unstable for
    // this scenario. The reconstruction keeps the exact hydrostatic value.
    const auto pressureLeftBoundary =
      pressure + 0.5 * density * -GRAVITY * h(normal);
    const auto pressureRightBoundary =
      pressure - 0.5 * density * -GRAVITY * h(normal);

    {{COMPUTE_PRECISION}} extrapolatedMinus[{{NUMBER_OF_UNKNOWNS}}];
    {{COMPUTE_PRECISION}} extrapolatedPlus[{{NUMBER_OF_UNKNOWNS}}];
    for (int component = 0; component < {{NUMBER_OF_UNKNOWNS}}; component++) {
      extrapolatedMinus[component] = Q[component] - 0.5 * limitedDifferences[component];
      extrapolatedPlus[component] =
        Q[component] + 0.5 * limitedDifferences[component];
    }

    {{COMPUTE_PRECISION}} velocitySquaredLeftBoundary;
    {{COMPUTE_PRECISION}} velocitySquaredRightBoundary;
    computeVelocitySquared(extrapolatedMinus, velocitySquaredLeftBoundary);
    computeVelocitySquared(extrapolatedPlus, velocitySquaredRightBoundary);

    reconstructedMinus =
      pressureLeftBoundary / (GAMMA - 1.0)
      + 0.5 * extrapolatedMinus[ShortcutRho]
        * velocitySquaredLeftBoundary;
    reconstructedPlus =
      pressureRightBoundary / (GAMMA - 1.0)
      + 0.5 * extrapolatedPlus[ShortcutRho]
        * velocitySquaredRightBoundary;
  } else {
    reconstructedMinus = Q[unknown] - 0.5 * limitedDifferences[unknown];
    reconstructedPlus  = Q[unknown] + 0.5 * limitedDifferences[unknown];
  }
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="IsothermalStratification",
    dimensions=2,
    size=[40_000, 40_000, 40_000],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != 2:
    raise ValueError("The isothermal stratification test runs in 2D only.")

fv_solver = exahype2.fv.singlestep.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.MusclHancock(),
)

# The f-wave face solver includes gravity. MUSCL-Hancock also needs the
# cell-centred source update.
fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    limited_difference=limiter,
    reconstruction=reconstruction,
    riemann_solver=fWave,
    source=source_term,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.output.makefile.add_compile_definition("GRAVITY=9.81")
project.output.makefile.add_compile_definition("GAS_CONSTANT=8.31")
project.output.makefile.add_compile_definition("TEMPERATURE=299")
project.output.makefile.add_compile_definition("GROUND_DENSITY=1.166")
project.output.makefile.add_compile_definition("MEAN_MOLECULAR_MASS=0.0289647")
project.generate_and_build()
