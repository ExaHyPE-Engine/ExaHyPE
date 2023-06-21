#include "NavierStokesSolver.h"

tarch::logging::Log applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::_log(
  "applications::exahype2:"
  ":CompressibleNavierStok"
  "es::NavierStokesSolver"
);

enum class Scenario
{
  LidDrivenCavity,
  ChannelFlow,
  ChannelFlowWithObstacle,
  ShockTube,
  Bubbles,
  RayleighTaylorInstability,
  PointExplosion,
};

Scenario scenario = Scenario::LidDrivenCavity;
// Scenario scenario = Scenario::PointExplosion;

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructred
)
{
  assertion(!std::isnan(Q[rho]));
  assertion(!std::isnan(Q[u]));
  assertion(!std::isnan(Q[v]));
  assertion(!std::isnan(Q[e]));

  switch (scenario)
  {
  case Scenario::LidDrivenCavity:
    Q[rho] = 1.0;
    Q[u]   = 0.0;
    Q[v]   = 0.0;
    Q[e]   = (100 / Gamma) / (Gamma - 1) + 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho];
    break;
  case Scenario::PointExplosion:
    const tarch::la::Vector<2, double> circleCentre  = {0.5, 0.3};
    bool                               isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.05);
    Q[rho]                                           = 0.1;
    Q[u]                                             = 0;
    Q[v]                                             = 0;
    Q[e]                                             = isInTheCentre ? 1.0 : 0.05; // inner energy
    break;
  }
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::boundaryConditions(
  const double* __restrict__ Qinside,
  double* __restrict__ Qoutside,
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
)
{
  assertion(!std::isnan(Qinside[rho]));
  assertion(!std::isnan(Qinside[u]));
  assertion(!std::isnan(Qinside[v]));
  assertion(!std::isnan(Qinside[e]));

  switch (scenario)
  {
  case Scenario::LidDrivenCavity:
    Qoutside[rho] = Qinside[rho];
    Qoutside[u]   = 0;
    if (faceCentre[y] >= DomainSize[y] - std::numeric_limits<double>::epsilon())
    { // TODO: DomainOffset missing here
      Qoutside[u] = LidDrivenCavityVelocity;
    }
    Qoutside[v] = 0;
    Qoutside[e] = Qinside[e];
    break;
  case Scenario::PointExplosion:
    Qoutside[rho] = Qinside[rho];
    Qoutside[u]   = Qinside[u];
    Qoutside[v]   = Qinside[v];
    Qoutside[e]   = Qinside[e];
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
)
{
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
)
{
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
)
{
  nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ, Offloadable::Yes);
}

::exahype2::RefinementCommand applications::exahype2::CompressibleNavierStokes::NavierStokesSolver::refinementCriterion(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t
)
{
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  switch (scenario)
  {
  case Scenario::LidDrivenCavity:
    result = volumeCentre[y] > DomainSize[y] / 2 ? ::exahype2::RefinementCommand::Refine : result;
    break;
  }

  return result;
}
