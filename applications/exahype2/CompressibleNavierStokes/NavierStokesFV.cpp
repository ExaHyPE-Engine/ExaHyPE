#include "NavierStokesFV.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log applications::exahype2::CompressibleNavierStokes::NavierStokesFV::_log("applications::exahype2::CompressibleNavierStokes::NavierStokesFV");

void applications::exahype2::CompressibleNavierStokes::NavierStokesFV::initialCondition(
    double *__restrict__ Q,
    const tarch::la::Vector<Dimensions, double> &volumeCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    bool gridIsConstructred)
{
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructred);

  // @todo Implement your stuff here, but ensure all entries are
  //   initialised incl all auxiliary variables. The snippet below
  //   sets everything to zero
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++)
  {
    Q[i] = 0.0;
  }

  logTraceOut("initialCondition(...)");
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesFV::boundaryConditions(
    const double *__restrict__ Qinside, // Qinside[4+0]
    double *__restrict__ Qoutside,      // Qoutside[4+0]
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    int normal)
{
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);
  // @todo implement
  logTraceOut("boundaryConditions(...)");
}

double ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::maxEigenvalue(
    const double *__restrict__ Q, // Q[4+0],
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal)
{
  logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);
  const double gamma = 1.4;
  double p = (gamma - 1) * (Q[3] - 0.5 * (Q[1] * Q[1] + Q[2] * Q[2]) / Q[0]);
  double c = std::sqrt(gamma * p / Q[0]);
  logTraceOut("maxEigenvalue(...)");
  return std::abs(Q[1 + normal] / Q[0]) + c;
}

void ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::flux(
    const double *__restrict__ Q, // Q[4+0],
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal,
    double *__restrict__ F // F[4]
)
{
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);
  const double gamma = 1.4;
  double p = (gamma - 1) * (Q[3] - 0.5 * (Q[1] * Q[1] + Q[2] * Q[2]) / Q[0]);
  F[0] = Q[1 + normal];
  if (normal == 0)
  {
    F[1] = Q[1] * Q[1] / Q[0] + p;
    F[2] = Q[1] * Q[2] / Q[0];
  }
  else
  {
    F[1] = Q[1] * Q[2] / Q[0];
    F[2] = Q[2] * Q[2] / Q[0] + p;
  }
  F[3] = Q[1 + normal] * (Q[3] + p) / Q[0];
  logTraceOut("flux(...)");
}

void ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::nonconservativeProduct(
    const double *__restrict__ Q,      // Q[4+0],
    const double *__restrict__ deltaQ, // deltaQ[4+0]
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal,
    double *__restrict__ BTimesDeltaQ // BTimesDeltaQQ[4]
)
{
  logTraceInWith4Arguments("nonconservativeProduct(...)", faceCentre, volumeH, t, normal);
  // @todo implement
  logTraceOut("nonconservativeProduct(...)");
}

void ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::sourceTerm(
    const double *__restrict__ Q, // Q[4+0]
    const tarch::la::Vector<Dimensions, double> &volumeX,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    double *__restrict__ S // S[4]
)
{
  logTraceInWith4Arguments("sourceTerm(...)", volumeX, volumeH, t, dt);

  // @todo implement and ensure that all entries of S are properly set
  for (int i = 0; i < NumberOfUnknowns; i++)
  {
    S[i] = 0.0;
  }

  logTraceOut("sourceTerm(...)");
}
