#include "AcousticWaveRKDG.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log applications::exahype2::AcousticWave::AcousticWaveRKDG::_log("applications::exahype2::AcousticWave::AcousticWaveRKDG");

void applications::exahype2::AcousticWave::AcousticWaveRKDG::initialCondition(
    double *__restrict__ Q,
    const tarch::la::Vector<Dimensions, double> &x,
    const tarch::la::Vector<Dimensions, double> &h,
    const tarch::la::Vector<Dimensions,int> &index,
    bool gridIsConstructred)
{
  logTraceInWith3Arguments("initialCondition(...)", x, h, gridIsConstructred);

  // @todo Implement your stuff here, but ensure all entries are
  //   initialised incl all auxiliary variables. The snippet below
  //   sets everything to zero
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++)
  {
    Q[i] = 0.0;
  }
  logTraceOut("initialCondition(...)");
}

void applications::exahype2::AcousticWave::AcousticWaveRKDG::boundaryConditions(
    const double *__restrict__ Qinside, // Qinside[3+0]
    double *__restrict__ Qoutside,      // Qoutside[3+0]
    const tarch::la::Vector<Dimensions, double> &x,
    double t,
    int normal)
{
  logTraceInWith3Arguments("boundaryConditions(...)", x, t, normal);
  Qoutside[0] = 0.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  logTraceOut("boundaryConditions(...)");
}

double ::applications::exahype2::AcousticWave::AcousticWaveRKDG::maxEigenvalue(
    const double *__restrict__ Q, // Q[3+0],
    const tarch::la::Vector<Dimensions, double> &x,
    double t,
    double dt,
    int normal)
{
  logTraceInWith3Arguments("maxEigenvalue(...)", x, t, normal);
  return 6.0;
  logTraceOut("maxEigenvalue(...)");
}

void ::applications::exahype2::AcousticWave::AcousticWaveRKDG::flux(
    const double *__restrict__ Q, // Q[3+0],
    const tarch::la::Vector<Dimensions, double> &x,
    double t,
    double dt,
    int normal,
    double *__restrict__ F // F[3]
)
{
  logTraceInWith3Arguments("flux(...)", x, t, normal);

  double rho = 2.7;
  double c = 6.0;
  double K_0 = rho * c * c;

  if (normal == 0)
  {
    F[0] = -K_0 * Q[1];
    F[1] = -1 / rho * Q[0];
    F[2] = 0.0;
  }
  else
  {
    F[0] = -K_0 * Q[2];
    F[1] = 0.0;
    F[2] = -1 / rho * Q[0];
  }
  logTraceOut("flux(...)");
}

void ::applications::exahype2::AcousticWave::AcousticWaveRKDG::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    )
    {
      logTraceInWith3Arguments("sourceTerm(...)", x, t, dt);

      tarch::la::Vector<Dimensions,double> centre = {5, 5};
      bool nearCentre = tarch::la::norm2(x-centre)<0.1;
      if (nearCentre)
      {
        double sigma = 0.1149;
        double t0 = 0.7;
        double f = 0.0;
        double M0 = 1000.0;

        f = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * sigma * sigma));

        S[0] = f;
        S[1] = 0.0;
        S[2] = 0.0;
        logTraceOut("sourceTerm(...)");
    }
}
