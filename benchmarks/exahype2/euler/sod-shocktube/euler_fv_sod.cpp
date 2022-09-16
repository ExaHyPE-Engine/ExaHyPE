#include "euler_sod.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::_log( "examples::exahype2::test::euler_sod::benchmark::euler_sod" );

void benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );
  // Initial data
  constexpr double gamma = 1.39999999999999991118;
  constexpr double x_0 = 0.50000000000000000000;

  constexpr double rho_5 = 0.12500000000000000000; // right states
  constexpr double P_5 = 0.10000000000000000555;
  constexpr double u_5 = 0.00000000000000000000;
  constexpr double rho_1 = 1.00000000000000000000; // left states
  constexpr double P_1 = 1.00000000000000000000;
  constexpr double u_1 = 0.00000000000000000000;

  // Sound speed
  constexpr double cs_1 = 1.18321595661992318149;

  // Contact left
  constexpr double rho_3 = 0.42631942817849538541;
  constexpr double P_3 = 0.30313017805064701449;
  constexpr double u_3 = 0.92745262004895057117;
  constexpr double cs_3 = 0.99772543261013335592;

  // Contact right
  constexpr double rho_4 = 0.26557371170530713611;
  constexpr double P_4 = 0.30313017805064701449;
  constexpr double u_4 = 0.92745262004895057117;

  // Shock
  constexpr double u_shock = 1.75215573203017838111;

  // Key Positions
  const double x_4 = x_0; // position of shock
  const double x_3 = x_0; // position of contact discontinuity
  const double x_2 = x_0; // foot of rarefaction wave
  const double x_1 = x_0; // head of rarefaction wave

  double p = 0; // pressure
  Q[2] = 0;     // y momentum

  if (volumeCentre[0] < x_0)
  {
    Q[0] = rho_1;
    Q[1] = Q[0] * u_1; //x momentum
    p = P_1;
  }
  else
  {
    Q[0] = rho_5;
    Q[1] = Q[0] * u_5; //x momentum
    p = P_5;
  }
  Q[3] = p / (gamma - 1) + 0.5 / Q[0] * (Q[1] * Q[1]); // j*j, j=rho*v !!! ; assumes: Q[2]=0.

  logTraceOut( "initialCondition(...)" );
}

void benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}

double ::benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  const double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
  const double c = std::sqrt(gamma * p * irho);

  double result = 1.0;
  switch (normal)
  {
  case 0:
    result = std::max(std::abs(Q[1] + c), std::abs(Q[1] - c));
    break;
  case 1:
    result = std::max(std::abs(Q[2] + c), std::abs(Q[2] - c));
    break;
  }
  logTraceOut("maxEigenvalue(...)");
  return result;
}

void ::benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  const double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p =
      (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));

  switch (normal)
  {
  case 0:
    F[0] = Q[1];
    F[1] = irho * Q[1] * Q[1] + p;
    F[2] = irho * Q[1] * Q[2];
    F[3] = irho * Q[1] * (Q[3] + p);
    break;

  case 1:
    F[0] = Q[2];
    F[1] = irho * Q[2] * Q[1];
    F[2] = irho * Q[2] * Q[2] + p;
    F[3] = irho * Q[2] * (Q[3] + p);
    break;
  }
  logTraceOut( "flux(...)" );
}

    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod double ::benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  Offloadable
){  
    logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
    const double irho = 1.0 / Q[0];
    const double gamma = 1.4;
    const double p = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
    const double c = std::sqrt(gamma * p * irho);   
    double result = 1.0;
    switch (normal)
    {
    case 0:
      result = std::max(std::abs(Q[1] + c), std::abs(Q[1] - c));
      break;
    case 1:
      result = std::max(std::abs(Q[2] + c), std::abs(Q[2] - c));
      break;
    }
    logTraceOut("maxEigenvalue(...)");
    return result;
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif

    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void ::benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
){  
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  const double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p =
      (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));

  switch (normal)
  {
  case 0:
    F[0] = Q[1];
    F[1] = irho * Q[1] * Q[1] + p;
    F[2] = irho * Q[1] * Q[2];
    F[3] = irho * Q[1] * (Q[3] + p);
    break;

  case 1:
    F[0] = Q[2];
    F[1] = irho * Q[2] * Q[1];
    F[2] = irho * Q[2] * Q[2] + p;
    F[3] = irho * Q[2] * (Q[3] + p);
    break;
  }
  logTraceOut( "flux(...)" );
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif







