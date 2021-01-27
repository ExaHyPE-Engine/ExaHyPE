#include "Rusanov.h"
#include "Generic.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"

#include "peano4/utils/Loop.h"

#include "exahype2/NonCriticalAssertions.h"


void exahype2::fv::splitRusanov1d(
  std::function< void(
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  ) >   flux,
  std::function< double(
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  const double * __restrict__ QL,
  const double * __restrict__ QR,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       dx,
  double                                       t,
  double                                       dt,
  int                                          normal,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  double * __restrict__ FL,
  double * __restrict__ FR
) {
  assertion(normal>=0);
  assertion(normal<Dimensions);

  double * fluxFL = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);
  double * fluxFR = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);

  flux(QL,x,dx,t,dt,normal,fluxFL);
  flux(QR,x,dx,t,dt,normal,fluxFR);

  double lambdaMaxL = maxEigenvalue(QL,x,dx,t,dt,normal);
  double lambdaMaxR = maxEigenvalue(QR,x,dx,t,dt,normal);

  nonCriticalAssertion7( lambdaMaxL>=0.0, x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );
  nonCriticalAssertion7( lambdaMaxR>=0.0, x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );

  double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

  for (int unknown=0; unknown<unknowns; unknown++) {
    FL[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
    FR[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
    nonCriticalAssertion2( FL[unknown]==FR[unknown], FL[unknown], FR[unknown]);
  }

  ::tarch::freeMemory(fluxFL, tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(fluxFR, tarch::MemoryLocation::Heap);
};


void exahype2::fv::splitRusanov1d(
   std::function< void(
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  ) >   flux,
  std::function< void(
    const double * __restrict__                  Q,
    const double * __restrict__                  deltaQ,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ BgradQ
  ) >   nonconservativeProduct,
  std::function< double(
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  const double * __restrict__ QL,
  const double * __restrict__ QR,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       dx,
  double                                       t,
  double                                       dt,
  int                                          normal,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  double * __restrict__ FL,
  double * __restrict__ FR
) {
  assertion(normal>=0);
  assertion(normal<Dimensions);

  double * fluxFL   = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);
  double * fluxFR   = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);
  double * fluxNCP  = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);

  double * Qaverage = ::tarch::allocateMemory(unknowns+auxiliaryVariables, tarch::MemoryLocation::Heap);
  double * deltaQ   = ::tarch::allocateMemory(unknowns+auxiliaryVariables, tarch::MemoryLocation::Heap);

  flux(QL,x,dx,t,dt,normal,fluxFL);
  flux(QR,x,dx,t,dt,normal,fluxFR);

  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    Qaverage[unknown] = 0.5 * QL[unknown] + 0.5 * QR[unknown];
    deltaQ[unknown]   = QR[unknown] - QL[unknown];
  }
  nonconservativeProduct(Qaverage,deltaQ,x,dx,t,dt,normal,fluxNCP);

  double lambdaMaxL = maxEigenvalue(QL,x,dx,t,dt,normal);
  double lambdaMaxR = maxEigenvalue(QR,x,dx,t,dt,normal);

  nonCriticalAssertion7( lambdaMaxL>=0.0, x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );
  nonCriticalAssertion7( lambdaMaxR>=0.0, x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );

  double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

  for (int unknown=0; unknown<unknowns; unknown++) {
    // @todo We have to re-study the sign here.
    //FL[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) - 0.5 * fluxNCP[unknown];
    //FR[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) + 0.5 * fluxNCP[unknown];
    FL[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) + 0.5 * fluxNCP[unknown];
    FR[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) - 0.5 * fluxNCP[unknown];

    nonCriticalAssertion7( fluxNCP[unknown]==fluxNCP[unknown], x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );
  }

  ::tarch::freeMemory(fluxFL,   tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(fluxFR,   tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(fluxNCP,  tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(Qaverage, tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(deltaQ,   tarch::MemoryLocation::Heap);
};
