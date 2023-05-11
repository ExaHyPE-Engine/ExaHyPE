#include "NavierStokesSolver.h"

tarch::logging::Log applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::_log("applications::exahype2::CompressibleNavierStokes::NavierStokesSolver");

enum Coordinates {
  x = 0,
  y,
#if Dimensions == 3
  z
#endif
};

enum Variables {
  rho = 0,
  u,
  v,
#if Dimensions == 3
  w,
#endif
  e
};

enum Derivatives {
  drhodx = 0,
  drhody,
#if Dimensions == 3
  drhodz,
#endif
  dudx,
  dudy,
#if Dimensions == 3
  dudz,
#endif
  dvdx,
  dvdy,
#if Dimensions == 3
  dvdz,
  dwdx,
  dwdy,
  dwdz,
#endif
  dedx,
  dedy,
#if Dimensions == 3
  dedz
#endif
};

enum class Scenario {
  LidDrivenCavity,
  ChannelFlow,
  ChannelFlowWithObstacle, // "Karman Vortex Street" -> Obstacles can be easily added by adding an input reader for meshes (.stl) files.
  ShockTube,
  Bubbles,
  RayleighTaylorInstability,
};
Scenario scenario = Scenario::LidDrivenCavity;

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::initialCondition(
  [[maybe_unused]] double* __restrict__                         Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] bool                                         gridIsConstructred
) {
  assertion(!std::isnan(Q[rho]));
  assertion(!std::isnan(Q[u]));
  assertion(!std::isnan(Q[v]));
  assertion(!std::isnan(Q[e]));

  Q[rho] = 1.0;
  Q[u] = 0.0;
  Q[v] = 0.0;
  Q[e] = (100 / Gamma) / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::boundaryConditions(
  [[maybe_unused]] const double* __restrict__                   Qinside,
  [[maybe_unused]] double* __restrict__                         Qoutside,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] int                                          normal
) {
  assertion(!std::isnan(Qinside[rho]));
  assertion(!std::isnan(Qinside[u]));
  assertion(!std::isnan(Qinside[v]));
  assertion(!std::isnan(Qinside[e]));

  switch (scenario) {
  case Scenario::LidDrivenCavity:
    Qoutside[rho] = Qinside[rho];
    Qoutside[u] = 0;
    if (faceCentre[y] >= DomainSize[y] - std::numeric_limits<double>::epsilon()) { // TODO: DomainOffset missing here
      Qoutside[u] = LidDrivenCavityVelocity;
    }
    Qoutside[v] = 0;
    Qoutside[e] = Qinside[e];
    break;
  }
}

double applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  assertion(Q[rho] > 0);
  assertion(Q[e] > 0);
  assertion((v + normal) < (NumberOfUnknowns + NumberOfAuxiliaryVariables));
  assertion(!std::isnan(Q[rho]));
  assertion(!std::isnan(Q[u]));
  assertion(!std::isnan(Q[v]));
  assertion(!std::isnan(Q[e]));

  const double pressure = (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho]);
  assertion(pressure > 0);

  const double c_speedOfSound = std::sqrt(Gamma * pressure / Q[rho]);

  const double max_lambda_convective = std::abs(Q[u + normal] / Q[rho]) + c_speedOfSound;
  const double max_lambda_viscous = std::max(4.0 / 3.0 * Viscosity / Q[rho],
                                             Gamma * Viscosity / (PrandtlNumber * Q[rho]));

  return std::max(max_lambda_convective, max_lambda_viscous);
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::flux(
  [[maybe_unused]] const double* __restrict__                   Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F
) {
  assertion(Q[rho] > 0);
  assertion(Q[e] > 0);
  assertion((v + normal) < (NumberOfUnknowns + NumberOfAuxiliaryVariables));
  assertion(!std::isnan(Q[rho]));
  assertion(!std::isnan(Q[u]));
  assertion(!std::isnan(Q[v]));
  assertion(!std::isnan(Q[e]));

  const double pressure = (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho]);
  assertion(pressure > 0)

  F[rho] = Q[u + normal];
  F[e]   = Q[u + normal] * (Q[e] + pressure) / Q[rho];

  if (normal == 0) {
    F[u] = Q[u] * Q[u] / Q[rho] + pressure;
    F[v] = Q[u] * Q[v] / Q[rho];
  } else {
    F[u] = Q[u] * Q[v] / Q[rho];
    F[v] = Q[v] * Q[v] / Q[rho] + pressure;
  }
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q,
  [[maybe_unused]] const double* __restrict__                   deltaQ,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ
) {
  assertion(Q[rho] > 0);
  assertion(!std::isnan(Q[rho]));
  assertion(!std::isnan(Q[u]));
  assertion(!std::isnan(Q[v]));
  assertion(!std::isnan(Q[e]));
  assertion(!std::isnan(deltaQ[drhodx]));
  assertion(!std::isnan(deltaQ[drhody]));
  assertion(!std::isnan(deltaQ[dudx]));
  assertion(!std::isnan(deltaQ[dudy]));
  assertion(!std::isnan(deltaQ[dvdx]));
  assertion(!std::isnan(deltaQ[dvdy]));
  assertion(!std::isnan(deltaQ[dedx]));
  assertion(!std::isnan(deltaQ[dedy]));

  if (normal == 0) {
    // mu{2/3(du/dx+dv/dy)-2(du/dx)}
    BTimesDeltaQ[u] = Viscosity * (2.0 / 3.0 * (deltaQ[dudx] / volumeH[x] + deltaQ[dvdy] / volumeH[y]) - 2 * deltaQ[dudx] / volumeH[x]) / Q[rho];

    // mu{dv/dx + du/dy}
    BTimesDeltaQ[v] = /*-*/ Viscosity * (deltaQ[dvdx] / volumeH[x] + deltaQ[dudy] / volumeH[y]) / Q[rho];
  } else {
    // mu{dv/dx + du/dy}
    BTimesDeltaQ[u] = /*-*/ Viscosity * (deltaQ[dvdx] / volumeH[x] + deltaQ[dudy] / volumeH[y]) / Q[rho];

    // mu{2/3(du/dx+dv/dy)-2(dv/dy)}
    BTimesDeltaQ[v] = Viscosity * (2.0 / 3.0 * (deltaQ[dudx] / volumeH[x] + deltaQ[dvdy] / volumeH[y]) - 2 * deltaQ[dvdy] / volumeH[y]) / Q[rho];
  }

  BTimesDeltaQ[rho] = 0;
  BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v]) / Q[rho];
}

::exahype2::RefinementCommand applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::refinementCriterion(
  [[maybe_unused]] const double* __restrict__                   Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  switch (scenario) {
  case Scenario::LidDrivenCavity:
    result = volumeCentre[y] > DomainSize[y]/2 ? ::exahype2::RefinementCommand::Refine : result;
    break;
  }

  return result;
}
