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
        help="show this help message and continue with project help",
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

"""
@article{WellBalancedness,
	author = {{Käppeli, R.} and {Mishra, S.}},
	title = {A well-balanced finite volume scheme for the Euler equations  with gravitation - The exact preservation of hydrostatic equilibrium with arbitrary entropy stratification},
	DOI= "10.1051/0004-6361/201527815",
	url= "https://doi.org/10.1051/0004-6361/201527815",
	journal = {A&A},
	year = 2016,
	volume = 587,
	pages = "A94",
	month = "",
}

This artificial scenario describes a stratified atmosphere by ensuring that an exponentially decreasing density profile
is exactly balanced with an exponentially decreasing pressure gradient.
Therefore, no updates should occur over the duration of the simulation and nothing should change in the density
and pressure profiles. This scenario can be used to test the well-balancedness of the underlying numerical scheme
such that it accurately captures the initial steady state. This is particularly difficult for higher order methods,
such as the MUSCL-Hancock scheme.

The reconstruction proposed by Käppeli and Mishra aims at retaining an exponentially decaying pressure gradient
within the MUSCL-Hancock boundary extrapolation phase. To this end, they detail first- and second-order
pressure reconstruction techniques, which they find to capture the initial stratified steady state up to
machine precision in a Runge-Kutta scheme. While the first-order accurate pressure reconstruction is found to work
in this scenario, the necessary second-order reconstruction, which limits pressure perturbations after the initial
time step t = 0 to t != 0, results in unphysical updates.
In general, any sort of custom reconstruction of the volume boundary values using the limited slopes and original
values can be implemented with the newly introduced reconstruction argument of the MUSCL-Hancock solver.
"""

initial_conditions = """
  const auto scaleHeight  = (GAS_CONSTANT * TEMPERATURE) / GRAVITY / MEAN_MOLECULAR_MASS;
  const auto exponentialDistribution = tarch::la::pow(tarch::la::E, -x(1) / scaleHeight);

  Q[Shortcuts::rho]       = GROUND_DENSITY * exponentialDistribution;
  Q[Shortcuts::rhoU + 0]  = 0.0;
  Q[Shortcuts::rhoU + 1]  = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2]  = 0.0;
#endif

  const auto pressure = Q[Shortcuts::rho] * GAS_CONSTANT * TEMPERATURE / MEAN_MOLECULAR_MASS;
  Q[Shortcuts::rhoE] = pressure / (GAMMA - 1.0);
"""

boundary_conditions = """
  // normal == 0 => left
  // normal == 1 => bottom
  // normal == 2 => right
  // normal == 3 => top

  if (normal == 1 || normal == 3) {
    // positive dy upwards, negative dy downwards
    const auto dy = (normal == 3 ? h(1) : -h(1));

    const auto scaleHeight = GAS_CONSTANT * TEMPERATURE / GRAVITY / MEAN_MOLECULAR_MASS;
    const auto exponentialDistribution = tarch::la::pow(tarch::la::E, -dy / scaleHeight);

    const auto densityOutside   = Qinside[Shortcuts::rho] * exponentialDistribution;
    const auto pressureOutside  = densityOutside * GAS_CONSTANT * TEMPERATURE / MEAN_MOLECULAR_MASS;

    Qoutside[Shortcuts::rho] = densityOutside;
    const auto u = Qinside[Shortcuts::rhoU + 0] / Qinside[Shortcuts::rho];
    const auto v = Qinside[Shortcuts::rhoU + 1] / Qinside[Shortcuts::rho];
#if DIMENSIONS == 3
    const auto w = Qinside[Shortcuts::rhoU + 2] / Qinside[Shortcuts::rho];
    const auto uSq = u * u + v * v + w * w;
#else
    const auto uSq = u * u + v * v;
#endif

    Qoutside[Shortcuts::rhoE] = pressureOutside / (GAMMA - 1) + 0.5 * densityOutside * uSq;

    Qoutside[Shortcuts::rhoU + 0] = u * densityOutside;
#if DIMENSIONS == 3
    Qoutside[Shortcuts::rhoU + 2] = w * densityOutside;
#endif

    if (normal == 1) {
      // Reflective at bottom to keep atmosphere
      // from escaping the domain through the Earth's surface
      Qoutside[Shortcuts::rhoU + 1] = -v * densityOutside;
    } else {
      // Outflow at top
      Qoutside[Shortcuts::rhoU + 1] = v * densityOutside;
    }
  } else {
    // outflow in other directions than stratified
    for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
      Qoutside[unknown] = Qinside[unknown];
    }
  }
"""

compute_usq = """
  auto computeUSq = [] (
    const {{COMPUTE_PRECISION}}* const NOALIAS Q,
    {{COMPUTE_PRECISION}}& uSq
  ) {
    const int ShortcutRho = 0;
    const int ShortcutRhoU = 1;

    const auto u    = Q[ShortcutRhoU + 0] / Q[ShortcutRho];
    const auto v    = Q[ShortcutRhoU + 1] / Q[ShortcutRho];
    #if DIMENSIONS == 3
    const auto w    = Q[ShortcutRhoU + 2] / Q[ShortcutRho];
    uSq  = u * u + v * v + w * w;
    #else
    uSq  = u * u + v * v;
    #endif
  };
"""

calculate_pressure = (
    """
  auto calculatePressure = [] (
    const {{COMPUTE_PRECISION}}* const NOALIAS Q,
    {{COMPUTE_PRECISION}}& pressure,
    {{COMPUTE_PRECISION}}& uSq
  ) {
    const int ShortcutRho = 0;
    const int ShortcutRhoE = DIMENSIONS + 1;

    """
    + compute_usq
    + """

    computeUSq(Q, uSq);
    pressure = (GAMMA - 1.0) * (Q[ShortcutRhoE] - 0.5 * Q[ShortcutRho] * uSq);
  };
"""
)

calculate_pressure_perturbations = (
    """
  auto pressurePerturbations = [] (
    const {{COMPUTE_PRECISION}}* const NOALIAS QL,
    const {{COMPUTE_PRECISION}}* const NOALIAS Q,
    const {{COMPUTE_PRECISION}}* const NOALIAS QR,
    const tarch::la::Vector<DIMENSIONS, double> h,
    const int normal,
    {{COMPUTE_PRECISION}}& pressurePerturbationLeft,
    {{COMPUTE_PRECISION}}& pressurePerturbationRight
  ) {
    /**
     * Following "A well-balanced finite volume scheme for the Euler equations with gravitation", R. Käppeli and S. Mishra, https://doi.org/10.1051/0004-6361/201527815 
     */
    const auto densityLeft  = QL[0];
    const auto density      = Q[0];
    const auto densityRight = QR[0];

    """
    + calculate_pressure
    + """

    {{COMPUTE_PRECISION}} pressure;
    {{COMPUTE_PRECISION}} uSq;
    calculatePressure(Q, pressure, uSq);

    {{COMPUTE_PRECISION}} pressureLeft;
    {{COMPUTE_PRECISION}} uSqLeft;
    calculatePressure(QL, pressureLeft, uSqLeft);

    {{COMPUTE_PRECISION}} pressureRight;
    {{COMPUTE_PRECISION}} uSqRight;
    calculatePressure(QR, pressureRight, uSqRight);

    // Eq. 26
    const auto reconstructedPressureLeft   = pressure + 0.5 * (densityLeft + density) * -GRAVITY * h(normal);
    const auto reconstructedPressureRight  = pressure - 0.5 * (densityRight + density) * -GRAVITY * h(normal);

    // Eq. 25
    pressurePerturbationLeft  = pressureLeft - reconstructedPressureLeft;
    pressurePerturbationRight = pressureRight - reconstructedPressureRight;
  };
"""
)

limiter = (
    calculate_pressure_perturbations
    + """
  {{COMPUTE_PRECISION}} a;
  {{COMPUTE_PRECISION}} b;
  if (unknown == 3 && normal == 1) {
    pressurePerturbations(QL, Q, QR, h, normal, b, a);
  } else {
    a = QR[unknown] - Q[unknown];
    b = Q[unknown] - QL[unknown];
  }

  // Minmod
  if (a * b <= 0.0) {
    return 0.0;
  } else {
    if (tarch::la::smaller(tarch::la::abs(a), tarch::la::abs(b))) {
      return a;
    } else {
      return b;
    }
  }
"""
)

"""
Required in MUSCL-Hancock before we solve the Riemann Problem.
-> Not the same as a simple postprocessing step
"""
reconstruction = (
    """
  auto reconstructEnergyAtBoundary = [] (
    const {{COMPUTE_PRECISION}} * const NOALIAS Q,
    const tarch::la::Vector<DIMENSIONS, double> h,
    const int                                   normal,
    const {{COMPUTE_PRECISION}}                 slope,
    {{COMPUTE_PRECISION}}* const NOALIAS        extrapolatedBoundaryLeft,
    {{COMPUTE_PRECISION}}* const NOALIAS        extrapolatedBoundaryRight
  ) {

    """
    + calculate_pressure
    + """

    {{COMPUTE_PRECISION}} pressure;
    {{COMPUTE_PRECISION}} uSq;
    calculatePressure(Q, pressure, uSq);

    const int ShortcutRho = 0;
    const int ShortcutE = DIMENSIONS + 1;

    const {{COMPUTE_PRECISION}} density = Q[ShortcutRho];

    // Eq. 16 Käppeli and Mishra
    const auto reconstructedPressureLeftBoundary  = pressure + 0.5 * density * -GRAVITY * h(normal); 
    const auto reconstructedPressureRightBoundary = pressure - 0.5 * density * -GRAVITY * h(normal);

    // Eq. 27 Käppeli and Mishra
    // Following p_{1,i}(x_i) = p_i - p_{0,i}(x_i) = 0, all pressure perturbations are zero, 
    // so we just add the perturbation slopes to the reconstructed pressure
    const auto pressureLeftBoundary  = reconstructedPressureLeftBoundary - 0.5 * h(normal) * slope;
    const auto pressureRightBoundary = reconstructedPressureRightBoundary + 0.5 * h(normal) * slope;

    // compute boundary uSqs
    {{COMPUTE_PRECISION}} uSqLeftBoundary;
    {{COMPUTE_PRECISION}} uSqRightBoundary;

    """
    + compute_usq
    + """

    computeUSq(extrapolatedBoundaryLeft,  uSqLeftBoundary);
    computeUSq(extrapolatedBoundaryRight, uSqRightBoundary);

    // convert pressure back to energy
    extrapolatedBoundaryLeft[ShortcutE]  = pressureLeftBoundary / (GAMMA - 1.0) + 0.5 * extrapolatedBoundaryLeft[ShortcutRho] * uSqLeftBoundary;
    extrapolatedBoundaryRight[ShortcutE] = pressureRightBoundary / (GAMMA - 1.0) + 0.5 * extrapolatedBoundaryRight[ShortcutRho] * uSqRightBoundary;
  };

  if (unknown == 3 && normal == 1) {
    reconstructEnergyAtBoundary(QInGathered,      volumeSize, normal, slopeGathered[normal][unknown],       extrapolatedBoundaryLeft,                       extrapolatedBoundaryRight);
    reconstructEnergyAtBoundary(QInGatheredLeft,  volumeSize, normal, slopeGatheredLeft[normal][unknown],   extrapolatedBoundaryLeftNeighborLeftBoundary,   extrapolatedBoundaryLeftNeighborRightBoundary);
    reconstructEnergyAtBoundary(QInGatheredRight, volumeSize, normal, slopeGatheredRight[normal][unknown],  extrapolatedBoundaryRightNeighborLeftBoundary,  extrapolatedBoundaryRightNeighborRightBoundary);
  } else {
    extrapolatedBoundaryLeft[unknown]                       = QInGathered[unknown] - 0.5 * volumeSize(normal) * slopeGathered[normal][unknown];
    extrapolatedBoundaryRight[unknown]                      = QInGathered[unknown] + 0.5 * volumeSize(normal) * slopeGathered[normal][unknown];
    extrapolatedBoundaryLeftNeighborLeftBoundary[unknown]   = QInGatheredLeft[unknown] - 0.5 * volumeSize(normal) * slopeGatheredLeft[normal][unknown];
    extrapolatedBoundaryLeftNeighborRightBoundary[unknown]  = QInGatheredLeft[unknown] + 0.5 * volumeSize(normal) * slopeGatheredLeft[normal][unknown];
    extrapolatedBoundaryRightNeighborLeftBoundary[unknown]  = QInGatheredRight[unknown] - 0.5 * volumeSize(normal) * slopeGatheredRight[normal][unknown];
    extrapolatedBoundaryRightNeighborRightBoundary[unknown] = QInGatheredRight[unknown] + 0.5 * volumeSize(normal) * slopeGatheredRight[normal][unknown];
  }
"""
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="IsothermalStratification",
    dimensions=2,
    size=[40_000, 40_000, 40_000],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
    reconstruction=reconstruction,  # Specific to MUSCL-Hancock
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    limiter=limiter,
    riemann_solver=fWave,
)

########################################################
# Source term needs to be commented out for Godunov.   #
# Otherwise, the effects of the source term will be    #
# taken into account twice (f-wave and cell centered). #
# MUSCL-Hancock needs the source term in both places.  #
########################################################
fv_solver.set_implementation(
    source_term=source_term,
)

project.add_solver(fv_solver)

project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.output.makefile.add_CXX_flag("-DGRAVITY=9.81")
project.output.makefile.add_CXX_flag("-DGAS_CONSTANT=8.31")
project.output.makefile.add_CXX_flag("-DTEMPERATURE=299")
project.output.makefile.add_CXX_flag("-DGROUND_DENSITY=1.166")
project.output.makefile.add_CXX_flag("-DMEAN_MOLECULAR_MASS=0.0289647")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
