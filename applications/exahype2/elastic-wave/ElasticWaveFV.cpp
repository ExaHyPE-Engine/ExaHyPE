#include "ElasticWaveFV.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log applications::exahype2::ElasticWave::ElasticWaveFV::_log("applications::exahype2::ElasticWave::ElasticWaveFV");

::exahype2::RefinementCommand applications::exahype2::ElasticWave::ElasticWaveFV::refinementCriterion(
    const double *__restrict__ Q,
    const tarch::la::Vector<Dimensions, double> &volumeX,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t)
{
  logTraceInWith3Arguments("refinementCriterion(...)", volumeX, volumeH, t);
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if (Dimensions == 2)
  {
    if (tarch::la::equals(t, 0.0))
    {
      tarch::la::Vector<Dimensions, double> circleCentre = {5.0, 5.0};
      double distance = tarch::la::norm2(volumeX - circleCentre);

      bool nearCircle = (0.8 < distance && distance < 1.0);
      if (nearCircle)
      {
        result = ::exahype2::RefinementCommand::Refine;
      }
    }
  }
  else
  {
    if (tarch::la::equals(t, 0.0))
    {
      tarch::la::Vector<Dimensions, double> circleCentre = {5.0, 5.0, 5.0};
      double distance = tarch::la::norm2(volumeX - circleCentre);

      bool nearCircle = (0.8 < distance && distance < 1.0);
      if (nearCircle)
      {
        result = ::exahype2::RefinementCommand::Refine;
      }
    }
  }
  logTraceOutWith1Argument("refinementCriterion(...)", ::toString(result));
  return result;
}

void applications::exahype2::ElasticWave::ElasticWaveFV::initialCondition(
    double *__restrict__ Q,
    const tarch::la::Vector<Dimensions, double> &volumeCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    bool gridIsConstructred)
{
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructred);
  if (Dimensions == 2)
  {
    // stresses
    Q[0] = 0.0; // xx
    Q[1] = 0.0; // yy
    Q[2] = 0.0; // xy

    // velocities
    tarch::la::Vector<Dimensions, double> circleCentre = {5.0, 5.0};
    bool inCircle = (tarch::la::norm2(volumeCentre - circleCentre) < 1.5);

    Q[3] = inCircle ? std::exp(-tarch::la::norm2(volumeCentre - circleCentre) / 2) * 2 : 0; // x
    Q[4] = Q[3];                                                                            // y

    // auxiliary variables
    Q[5] = inCircle ? 4.0 : 6.0;   // rho
    Q[6] = inCircle ? 2.0 : 3.464; // cp
    Q[7] = inCircle ? 2.6 : 2.7;   // cs
  }
  else
  {
    // stresses
    Q[0] = 0.0; // xx
    Q[1] = 0.0; // yy
    Q[2] = 0.0; // zz
    Q[3] = 0.0; // xy
    Q[4] = 0.0; // xz
    Q[5] = 0.0; // yz

    // velocities
    tarch::la::Vector<Dimensions, double> circleCentre = {5.0, 5.0, 5.0};
    bool inCircle = (tarch::la::norm2(volumeCentre - circleCentre) < 1.5);

    Q[6] = inCircle ? std::exp(-tarch::la::norm2(volumeCentre - circleCentre) / 2) * 2 : 0; // x
    Q[7] = Q[6];                                                                            // y
    Q[8] = Q[6];                                                                            // z

    // auxiliary variables
    Q[9] = inCircle ? 4.0 : 6.0;    // rho
    Q[10] = inCircle ? 2.0 : 3.464; // cp
    Q[11] = inCircle ? 2.6 : 2.7;   // cs
  }

  logTraceOut("initialCondition(...)");
}

void applications::exahype2::ElasticWave::ElasticWaveFV::boundaryConditions(
    const double *__restrict__ Qinside,
    double *__restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    int normal)
{
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);
  if (Dimensions == 2)
  {
    // stresses
    Qoutside[0] = 0;
    Qoutside[1] = 0;
    Qoutside[2] = 0;

    // velocities
    Qoutside[3] = 0;
    Qoutside[4] = 0;

    // auxiliary variables
    Qoutside[5] = Qinside[5]; // rho
    Qoutside[6] = Qinside[6]; // cp
    Qoutside[7] = Qinside[7]; // cs
  }
  else
  {
    // stresses
    Qoutside[0] = 0;
    Qoutside[1] = 0;
    Qoutside[2] = 0;
    Qoutside[3] = 0;
    Qoutside[4] = 0;
    Qoutside[5] = 0;

    // velocities
    Qoutside[6] = 0;
    Qoutside[7] = 0;
    Qoutside[8] = 0;

    // auxiliary variables
    Qoutside[9] = Qinside[9];   // rho
    Qoutside[10] = Qinside[10]; // cp
    Qoutside[11] = Qinside[11]; // cs
  }
  logTraceOut("boundaryConditions(...)");
}

double ::applications::exahype2::ElasticWave::ElasticWaveFV::maxEigenvalue(
    const double *__restrict__ Q,
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal)
{
  logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);

  double result;
  if (Dimensions == 2)
  {
    // distinct eigenvalues are cp, -cp, cs, -cs, 0
    result = std::max(std::abs(Q[6]), std::abs(Q[7]));
  }
  else
  {
    // distinct eigenvalues are cp, -cp, cs, -cs, 0
    result = std::max(std::abs(Q[10]), std::abs(Q[11]));
  }
  logTraceOut("maxEigenvalue(...)");
  return result;
}

void ::applications::exahype2::ElasticWave::ElasticWaveFV::flux(
    const double *__restrict__ Q,
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal,
    double *__restrict__ F)
{
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);
  if (Dimensions == 2)
  {
    // stresses
    for (int i = 0; i < 3; i++)
    {
      F[i] = 0;
    }

    // velocities
    switch (normal)
    {
    case 0:
      F[3] = Q[0];
      F[4] = Q[2];
      break;
    case 1:
      F[3] = Q[2];
      F[4] = Q[1];
      break;
    }
  }
  else
  {
    // stresses
    for (int i = 0; i < 6; i++)
    {
      F[i] = 0;
    }

    // velocities
    switch (normal)
    {
    case 0:        // x
      F[6] = Q[0]; // xx
      F[7] = Q[3]; // xy
      F[8] = Q[4]; // xz
      break;
    case 1:        // y
      F[6] = Q[3]; // xy
      F[7] = Q[1]; // yy
      F[8] = Q[5]; // yz
      break;
    case 2:        // z
      F[6] = Q[4]; // xz
      F[7] = Q[5]; // yz
      F[8] = Q[2]; // zz
      break;
    }
  }
  logTraceOut("flux(...)");
}

void ::applications::exahype2::ElasticWave::ElasticWaveFV::nonconservativeProduct(
    const double *__restrict__ Q,
    const double *__restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double> &faceCentre,
    const tarch::la::Vector<Dimensions, double> &volumeH,
    double t,
    double dt,
    int normal,
    double *__restrict__ BgradQ)
{
  logTraceInWith4Arguments("nonconservativeProduct(...)", faceCentre, volumeH, t, normal);
  if (Dimensions == 2)
  {
    // Lamé parameters
    double mu = Q[5] * Q[7] * Q[7];              // rho*cs^2
    double lambda = Q[5] * Q[6] * Q[6] - 2 * mu; // rho*cp^2 - 2 mu

    // stresses
    switch (normal)
    {
    case 0: // x
      BgradQ[0] = (lambda + 2 * mu) * deltaQ[3];
      BgradQ[1] = lambda * deltaQ[3];
      BgradQ[2] = mu * deltaQ[4];
      break;
    case 1: // y
      BgradQ[0] = lambda * deltaQ[4];
      BgradQ[1] = (lambda + 2 * mu) * deltaQ[4];
      BgradQ[2] = mu * deltaQ[3];
      break;
    }

    // velocities
    BgradQ[3] = 0;
    BgradQ[4] = 0;
  }
  else
  {
    // Lamé parameters
    double mu = Q[9] * Q[11] * Q[11];              // rho*cs^2
    double lambda = Q[9] * Q[10] * Q[10] - 2 * mu; // rho*cp^2 - 2 mu

    // stresses
    switch (normal)
    {
    case 0:                                      // x
      BgradQ[0] = (lambda + 2 * mu) * deltaQ[6]; // xx
      BgradQ[1] = lambda * deltaQ[6];            // yy
      BgradQ[2] = lambda * deltaQ[6];            // zz
      BgradQ[3] = mu * deltaQ[7];                // xy
      BgradQ[4] = mu * deltaQ[8];                // xz
      BgradQ[5] = 0;                             // yz
      break;
    case 1:                                      // y
      BgradQ[0] = lambda * deltaQ[7];            // xx
      BgradQ[1] = (lambda + 2 * mu) * deltaQ[7]; // yy
      BgradQ[2] = lambda * deltaQ[7];            // zz
      BgradQ[3] = mu * deltaQ[6];                // xy
      BgradQ[4] = 0;                             // xz
      BgradQ[5] = mu * deltaQ[8];                // yz
      break;
    case 2:
      BgradQ[0] = lambda * deltaQ[8];            // xx
      BgradQ[1] = lambda * deltaQ[8];            // yy
      BgradQ[2] = (lambda + 2 * mu) * deltaQ[8]; // zz
      BgradQ[3] = 0;                             // xy
      BgradQ[4] = mu * deltaQ[6];                // xz
      BgradQ[5] = mu * deltaQ[7];                // yz
      break;
    }

    // velocities
    BgradQ[6] = 0;
    BgradQ[7] = 0;
    BgradQ[8] = 0;
  }
  logTraceOut("nonconservativeProduct(...)");
}
