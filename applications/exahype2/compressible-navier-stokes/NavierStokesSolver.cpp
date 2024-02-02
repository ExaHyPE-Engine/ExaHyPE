#include "NavierStokesSolver.h"

tarch::logging::Log applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::_log(
  "applications::exahype2:"
  ":CompressibleNavierStok"
  "es::NavierStokesSolver"
);

enum class Scenario {
  LidDrivenCavity,
  ChannelFlow,
  ChannelFlowWithObstacle,
  ShockTube,
  DoubleShockTube,
  Bubbles,
  RayleighTaylorInstability,
  PointExplosion,
  SmoothWave,
  CJDetonation,
  TaylorGreenVortex,
};

Scenario scenario = Scenario::CJDetonation;

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructed
) {
  assertion(!std::isnan(Q[rho]));
  assertion(!std::isnan(Q[u]));
  assertion(!std::isnan(Q[v]));
  assertion(!std::isnan(Q[e]));

  switch (scenario) {

  case Scenario::LidDrivenCavity: {
    Q[rho] = 1.0;
    Q[u]   = 0.0;
    Q[v]   = 0.0;
#if Dimensions == 3
    Q[w] = 0.0;
#endif
    Q[e] = (100 / Gamma) / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
    Q[Z] = 0.0;
    break;
  }

  case Scenario::PointExplosion: {
#if Dimensions == 2
    const tarch::la::Vector<2, double> circleCentre  = {0.5, 0.5};
    bool                               isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.2);
#endif
#if Dimensions == 3
    const tarch::la::Vector<3, double> circleCentre  = {0.5, 0.5, 0.5};
    bool                               isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.2);
#endif
    Q[rho] = 1.0;
    Q[u]   = 0;
    Q[v]   = 0;
#if Dimensions == 3
    Q[w] = 0.0;
#endif
    Q[e] = isInTheCentre ? 1.00 : 1.01; // inner energy
    break;
  }

  case Scenario::TaylorGreenVortex: {
#if Dimensions == 2
    const double pressure = 0.25 * (std::cos(4.0 * M_PI * volumeCentre[0]) + std::sin(4.0 * M_PI * volumeCentre[1]))
                            + 100 / Gamma;
    Q[rho] = 1.0;
    Q[u]   = std::sin(2.0 * M_PI * volumeCentre[0]) * std::cos(2.0 * M_PI * volumeCentre[1]);
    Q[v]   = -std::cos(2.0 * M_PI * volumeCentre[0]) * std::sin(2.0 * M_PI * volumeCentre[1]);
    Q[e]   = pressure / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
#else
    std::cout << "Taylor green vortex only works for 2D!\n";
    exit(0);
#endif
    break;
  }

  case Scenario::ShockTube: {
    double pressure;
    if (volumeCentre[0] < 0.45) {
      Q[rho]   = 0.125;
      pressure = 0.1;
    } else {
      Q[rho]   = 1.0;
      pressure = 1.0;
    }
    Q[u] = 0;
    Q[v] = 0;
#if Dimensions == 2
    Q[e] = pressure / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
#else
    Q[w] = 0;
    Q[e] = pressure / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) / Q[rho];
#endif
    break;
  }

  case Scenario::DoubleShockTube: {
    double pressure;
    if (volumeCentre[0] < 0.33 || volumeCentre[0] > 0.66) {
      Q[rho]   = 0.125;
      pressure = 0.1;
    } else {
      Q[rho]   = 1.0;
      pressure = 1.0;
    }
    Q[u] = 0;
    Q[v] = 0;
#if Dimensions == 2
    Q[e] = pressure / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
#else
    Q[w] = 0;
    Q[e] = pressure / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) / Q[rho];
#endif
    break;
  }

  case Scenario::SmoothWave: {
    const auto   distX    = volumeCentre[0] - 0.5;
    const auto   distY    = volumeCentre[1] - 0.5;
    const auto   dist     = distX * distX + distY * distY;
    const double pressure = 1.0 - dist;

    Q[rho] = 1.0 - dist;
    Q[u]   = 0.0;
    Q[v]   = 0.0;
    Q[e]   = pressure / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
#if Dimensions == 3
    Q[w] = 0.0;
    Q[e] += 0.5 * Q[w] * Q[w] / Q[rho];
#endif
    break;
  }

  case Scenario::CJDetonation: {

#if Dimensions == 2
    // B: Burned, U: Unburned
    const auto rhoB = 1.4;
    const auto rhoU = 0.887565;
    const auto pB   = 1.0;
    const auto pU   = 0.191709;
    const auto ZB   = 0.0;
    const auto ZU   = 1.0;

    const auto alpha = std::atan2(volumeCentre[1], volumeCentre[0]); // Angle in polar coordinates
    const auto uB    = 0.0;
    const auto uU    = -0.577350 * std::cos(alpha);
    const auto vB    = 0.0;
    const auto vU    = -0.577350 * std::sin(alpha);

    const auto distX        = volumeCentre[0];
    const auto distY        = volumeCentre[1];
    const auto distToCenter = std::sqrt(distX * distX + distY * distY);
    const auto radiusBurned = 0.3;
    const auto isBurned     = distToCenter < radiusBurned;

    if (isBurned) {
      Q[rho] = rhoB;
      Q[u]   = uB * rhoB;
      Q[v]   = vB * rhoB;
      Q[Z]   = ZB * rhoB;
      Q[e]   = pB / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho] + Q[Z];
    } else {
      Q[rho] = rhoU;
      Q[u]   = uU * rhoU;
      Q[v]   = vU * rhoU;
      Q[Z]   = ZU * rhoU;
      Q[e]   = pU / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho] + Q[Z];
    }
    break;
#else
    std::cout << "CJ Detonation Wave Only works in 2D!\n";
    exit(0);
#endif
  }
  }
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  assertion(!std::isnan(Qinside[rho]));
  assertion(!std::isnan(Qinside[u]));
  assertion(!std::isnan(Qinside[v]));
  assertion(!std::isnan(Qinside[e]));

  switch (scenario) {
  case Scenario::LidDrivenCavity:
    Qoutside[rho] = Qinside[rho];
    Qoutside[u]   = 0;
    if (faceCentre[y] >= DomainSize[y] - std::numeric_limits<double>::epsilon()) { // TODO: DomainOffset missing here
      Qoutside[u] = LidDrivenCavityVelocity;
    }
    Qoutside[v] = 0;
#if Dimensions == 3
    Qoutside[w] = 0;
#endif
    Qoutside[e] = Qinside[e];
    break;

  case Scenario::CJDetonation:
  case Scenario::ShockTube:
  case Scenario::DoubleShockTube:
  case Scenario::PointExplosion:
    for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
      Qoutside[i] = Qinside[i];
    }
    break;

  case Scenario::SmoothWave:
    for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
      Qoutside[i] = Qinside[i];
    }
    if (normal != 0) {
      Qoutside[u + normal] = -Qinside[u + normal];
    }
    break;
  case Scenario::TaylorGreenVortex:
    const double pressure = 0.25 * std::exp(-4 * Viscosity * t)
                              * (std::cos(4.0 * M_PI * faceCentre[0]) + std::sin(4.0 * M_PI * faceCentre[1]))
                            + 100 / Gamma;

    Qoutside[rho] = 1.0;
    Qoutside[u]   = std::exp(-2 * Viscosity * t) * std::sin(2.0 * M_PI * faceCentre[0])
                  * std::cos(2.0 * M_PI * faceCentre[1]);
    Qoutside[v] = std::exp(-2 * Viscosity * t) * -std::cos(2.0 * M_PI * faceCentre[0])
                  * std::sin(2.0 * M_PI * faceCentre[1]);
    Qoutside[e] = pressure / (Gamma - 1) + 0.5 * (Qoutside[u] * Qoutside[u] + Qoutside[v] * Qoutside[v]) / Qoutside[rho];
    break;
  }
}

double applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::maxEigenvalue(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  return maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, Offloadable::Yes);
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::flux(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F
) {
  flux(Q, faceCentre, volumeH, t, dt, normal, F, Offloadable::Yes);
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::nonconservativeProduct(
  const double* __restrict__ Q,
  const double* __restrict__ deltaQ,
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BTimesDeltaQ
) {
  nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ, Offloadable::Yes);
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::sourceTerm(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  double* __restrict__ S
) {
  if (scenario == Scenario::CJDetonation) {
    const double irho = 1.0 / Q[rho];
#if Dimensions == 2
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v]) - Q[Z]);
#else
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) - Q[Z]);
#endif
    const double T = pressure * irho / R;

    S[Z] = -Q[Z] * (T > IgnitionTemperature ? -1.0 / Timescale : 0.0);
  }
}

::exahype2::RefinementCommand applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::refinementCriterion(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  switch (scenario) {
  case Scenario::LidDrivenCavity:
    result = volumeCentre[y] > DomainSize[y] / 2 ? ::exahype2::RefinementCommand::Refine : result;
    break;
  }

  return result;
}