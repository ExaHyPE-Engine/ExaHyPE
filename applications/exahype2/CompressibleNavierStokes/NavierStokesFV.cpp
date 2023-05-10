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

  const double gamma = 1.4;
  const double p = 100 / gamma;                                      // pressure
  Q[0] = 1.0;                                                        // density
  Q[1] = 0.0;                                                        // x-momentum
  Q[2] = 0.0;                                                        // y-momentum
  Q[3] = p / (gamma - 1) + 0.5 * (Q[1] * Q[1] + Q[2] * Q[2]) / Q[0]; // energy
  Q[4] = 0.01;                                                       // viscosity of fluid
  Q[5] = 0.7;                                                        // prandtl number

  logTraceOut("initialCondition(...)");
}

void applications::exahype2::CompressibleNavierStokes::NavierStokesFV::boundaryConditions(
    const double *__restrict__ Qinside, // Qinside[4+1]
    double *__restrict__ Qoutside,      // Qoutside[4+1]
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    int normal)
{
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);

  const double gamma = 1.4;
  const double p = (gamma - 1) * (Qinside[3] - 0.5 * (Qinside[1] * Qinside[1] + Qinside[2] * Qinside[2]) / Qinside[0]);

  if (p < 0)
  {
    std::cout << "Pressure is negative\n";
    exit(0);
  }
  const double wall_velocity = 1.0;
  Qoutside[0] = Qinside[0];
  if (faceCentre[1] >= 0.99)
  {
    Qoutside[1] = Qinside[0] * wall_velocity;
  }
  else
  {
    Qoutside[1] = 0;
  }
  Qoutside[2] = 0;
  Qoutside[3] = Qinside[3];
  logTraceOut("boundaryConditions(...)");
}

double ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::maxEigenvalue(
    const double *__restrict__ Q, // Q[4+1],
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal)
{
  logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);
  const double gamma = 1.4;
  const double p = (gamma - 1) * (Q[3] - 0.5 * (Q[1] * Q[1] + Q[2] * Q[2]) / Q[0]); // pressure
  if (p < 0)
  {
    std::cout << "Pressure is negative\n";
    exit(0);
  }
  const double c = std::sqrt(gamma * p / Q[0]);                                        // speed of sound
  const double lambda_c = std::abs(Q[1 + normal] / Q[0]) + c;                          // max convective eigenvalue
  const double lambda_v = std::max(4 / 3 * Q[4] / Q[0], gamma * Q[4] / (Q[5] * Q[0])); // max viscous eigenvalue
  logTraceOut("maxEigenvalue(...)");
  return std::max(lambda_c, lambda_v);
}

void ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::flux(
    const double *__restrict__ Q, // Q[4+1],
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal,
    double *__restrict__ F // F[4]./
)
{
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);
  const double gamma = 1.4;
  const double p = (gamma - 1) * (Q[3] - 0.5 * (Q[1] * Q[1] + Q[2] * Q[2]) / Q[0]);

  if (p < 0)
  {
    std::cout << "Pressure is negative\n";
    exit(0);
  }

  F[0] = Q[1 + normal];
  F[3] = Q[1 + normal] * (Q[3] + p) / Q[0];

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
  logTraceOut("flux(...)");
}

void ::applications::exahype2::CompressibleNavierStokes::NavierStokesFV::nonconservativeProduct(
    const double *__restrict__ Q,      // Q[4+1],
    const double *__restrict__ deltaQ, // deltaQ[4+1]
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal,
    double *__restrict__ BTimesDeltaQ // BTimesDeltaQQ[4]
)
{
  logTraceInWith4Arguments("nonconservativeProduct(...)", faceCentre, volumeH, t, normal);

  const double gamma = 1.4;
  const double p = (gamma - 1) * (Q[3] - 0.5 * (Q[1] * Q[1] + Q[2] * Q[2]) / Q[0]);
  if (p < 0)
  {
    std::cout << "Pressure is negative\n";
    exit(0);
  }

  if (normal == 0)
  {
    BTimesDeltaQ[1] = Q[4] * (2.0 / 3.0 * (deltaQ[2] / volumeH[0] + deltaQ[5] / volumeH[1]) - 2 * deltaQ[2] / volumeH[0]) / Q[0]; // mu{2/3(du/dx+dv/dy)-2(du/dx)}
    BTimesDeltaQ[2] = -Q[4] * (deltaQ[4] / volumeH[0] + deltaQ[3] / volumeH[1]) / Q[0];                                           // mu{dv/dx + du/dy}
  }
  else
  {
    BTimesDeltaQ[1] = -Q[4] * (deltaQ[4] / volumeH[0] + deltaQ[3] / volumeH[1]) / Q[0];                                           // mu{dv/dx + du/dy}
    BTimesDeltaQ[2] = Q[4] * (2.0 / 3.0 * (deltaQ[2] / volumeH[0] + deltaQ[5] / volumeH[1]) - 2 * deltaQ[5] / volumeH[1]) / Q[0]; // mu{2/3(du/dx+dv/dy)-2(dv/dy)}
  }
  BTimesDeltaQ[0] = 0;
  BTimesDeltaQ[3] = (Q[1] * BTimesDeltaQ[1] + Q[2] * BTimesDeltaQ[2]) / Q[0];
  logTraceOut("nonconservativeProduct(...)");
}
