#include "Euler.h"

#include "exahype2/RefinementControl.h"

#include "tarch/NonCriticalAssertions.h"


tarch::logging::Log applications::exahype2::euler::selfgravitation::Euler::_log("examples::exahype2::selfgravitation::Euler");


::exahype2::RefinementCommand applications::exahype2::euler::selfgravitation::Euler::refinementCriterion(
  const double* __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t
) {
  logTraceInWith3Arguments("refinementCriterion(...)", volumeX, volumeH, t);
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument("refinementCriterion(...)", ::toString(result));
  return result;
}


void applications::exahype2::euler::selfgravitation::Euler::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructed);

// Manual offset to make the wave originate slightly to the left of the center --- helps
// to detect if wave is moving to the left or right
#if Dimensions == 2
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.3};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {0.18, 0.3, 0.6};
#endif

  // initial conditions
  bool isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.05);
  Q[0]               = 0.1; // rho
  Q[1]               = 0;   // velocities
  Q[2]               = 0;
  Q[3]               = 0;
  Q[4]               = isInTheCentre ? 1.0 : 0.0; // inner energy
  Q[5] = 0.0; // ensure coupling terms are properly initialised; otherwise, they will pollute the very first time step
  Q[6] = 0.0;
  Q[7] = 0.0;

  logTraceOut("initialCondition(...)");
}


void applications::exahype2::euler::selfgravitation::Euler::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[5+0]
  double* __restrict__ Qoutside,      // Qoutside[5+0]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);
  nonCriticalAssertion4(Qinside[0] == Qinside[0], faceCentre, volumeH, t, normal);
  nonCriticalAssertion4(Qinside[1] == Qinside[1], faceCentre, volumeH, t, normal);
  nonCriticalAssertion4(Qinside[2] == Qinside[2], faceCentre, volumeH, t, normal);
  nonCriticalAssertion4(Qinside[3] == Qinside[3], faceCentre, volumeH, t, normal);
  nonCriticalAssertion4(Qinside[4] == Qinside[4], faceCentre, volumeH, t, normal);

  nonCriticalAssertion5(Qinside[0] > 1e-12, faceCentre, volumeH, t, normal, Qinside[0]);
  assertion5(Qinside[0] > 1e-12, faceCentre, volumeH, t, normal, Qinside[0]);

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

  logTraceOut("boundaryConditions(...)");
}


double applications::exahype2::euler::selfgravitation::Euler::maxEigenvalue(
  const double* __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);

  assertion(normal >= 0);
  assertion(normal < Dimensions);
  // assertion( Q[0]>0.0 );

  if (Q[0] <= 0.0 or Q[0] != Q[0]) {
    ::tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "Q[0]>0", "density negative");
    assertion(false);
  }

  constexpr double gamma = 1.4;
  const double     irho  = 1. / Q[0];
#if Dimensions == 3
  const double p = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));
#else
  const double                          p            = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
#endif

  //  nonCriticalAssertion13( p>=0.0, p, Q[0], Q[1], Q[2], Q[3], Q[4], Q[5], Q[6], Q[7], faceCentre, volumeH, t, normal
  //  );
  const double c = std::sqrt(gamma * std::abs(p) * irho);

  const double u_n    = Q[normal + 1] * irho;
  double       result = std::max(std::abs(u_n - c), std::abs(u_n + c));
  nonCriticalAssertion14(
    result >= 0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal
  );

  logTraceOutWith1Argument("maxEigenvalue(...)", result);
  return result;
}


void applications::exahype2::euler::selfgravitation::Euler::flux(
  const double* __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[5]
) {
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);
  assertion4(normal >= 0, faceCentre, volumeH, t, normal);
  assertion4(normal < Dimensions, faceCentre, volumeH, t, normal);
  nonCriticalAssertion9(Q[0] == Q[0], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal);
  nonCriticalAssertion9(Q[1] == Q[1], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal);
  nonCriticalAssertion9(Q[2] == Q[2], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal);
  nonCriticalAssertion9(Q[3] == Q[3], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal);
  nonCriticalAssertion9(Q[4] == Q[4], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal);

  if (Q[0] <= 1e-12 or Q[0] != Q[0]) {
    std::ostringstream msg;
    msg << "density is negative"
        << ".faceCentre=" << faceCentre << ",volumeH=" << volumeH << ",normal=" << normal << ",Q[0]=" << Q[0];
    ::tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "Q[0]>0", msg.str());
    assertion(false);
    exit(-1);
  }

  constexpr double gamma = 1.4;
  const double     irho  = 1. / Q[0];
#if Dimensions == 3
  const double p = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));
#else
  const double                          p            = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
#endif

  const double coeff = irho * Q[normal + 1];
  F[0]               = coeff * Q[0];
  F[1]               = coeff * Q[1];
  F[2]               = coeff * Q[2];
  F[3]               = coeff * Q[3];
  F[4]               = coeff * Q[4];
  F[normal + 1] += p;
  F[4] += coeff * p;

  nonCriticalAssertion8(F[0] == F[0], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4]);
  nonCriticalAssertion8(F[1] == F[1], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4]);
  nonCriticalAssertion8(F[2] == F[2], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4]);
  nonCriticalAssertion8(F[3] == F[3], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4]);
  nonCriticalAssertion8(F[4] == F[4], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4]);

  logTraceOutWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);
}


void applications::exahype2::euler::selfgravitation::Euler::sourceTerm(
  const double* __restrict__ Q, // Q[5+3]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  double* __restrict__ S // S[5]
) {
  logTraceInWith4Arguments("sourceTerm(...)", volumeX, volumeH, t, dt);

  const double T_tau = 1.0 / 4.0 / tarch::la::PI / tarch::la::PI;
  const double G     = 6.67e-11;
  const double rho   = Q[4]; // This is the input term

  S[0] = 0.0;
  S[1] = Q[0] * Q[5 + 0];
  S[2] = Q[0] * Q[5 + 1];
  S[3] = Q[0] * Q[5 + 2];
  S[4] = -Q[0] * (Q[1 + 0] * Q[5 + 0] + Q[1 + 1] * Q[5 + 1] + Q[1 + 2] * Q[5 + 2]);

  logTraceOut("sourceTerm(...)");
}
