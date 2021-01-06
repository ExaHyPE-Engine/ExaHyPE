#include "Rusanov.h"
#include "Generic.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"

#include "peano4/utils/Loop.h"

#include "exahype2/NonCriticalAssertions.h"


#if defined(GPUOffloading)
#pragma omp declare target
#endif
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

#if defined(GPUOffloading)
  double * fluxFL = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::ManagedAcceleratorMemory);
  double * fluxFR = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::ManagedAcceleratorMemory);
#else
  double * fluxFL = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);
  double * fluxFR = ::tarch::allocateMemory(unknowns, tarch::MemoryLocation::Heap);
#endif
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

#if defined(GPUOffloading)
  ::tarch::freeMemory(fluxFL, tarch::MemoryLocation::ManagedAcceleratorMemory);
  ::tarch::freeMemory(fluxFR, tarch::MemoryLocation::ManagedAcceleratorMemory);
#else
  ::tarch::freeMemory(fluxFL, tarch::MemoryLocation::Heap);
  ::tarch::freeMemory(fluxFR, tarch::MemoryLocation::Heap);
#endif
};
#if defined(GPUOffloading)
#pragma omp end declare target
#endif


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
    const double                                 gradQ[][Dimensions],
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

  double fluxFL[unknowns];
  double fluxFR[unknowns];
  flux(QL,x,dx,t,dt,normal,fluxFL);
  flux(QR,x,dx,t,dt,normal,fluxFR);

  double Qaverage[unknowns+auxiliaryVariables];
  double gradQ[unknowns+auxiliaryVariables][Dimensions];
  for (int unknown=0; unknown<unknowns+auxiliaryVariables; unknown++) {
    for (int d=0; d<Dimensions; d++) {
      gradQ[unknown][d] = 0.0;
    }
    Qaverage[unknown] = 0.5 * QL[unknown] + 0.5 * QR[unknown];
    gradQ[unknown][normal] = QR[unknown] - QL[unknown];
  }
  double fluxnonconservativeProduct[unknowns];
  nonconservativeProduct(Qaverage,gradQ,x,dx,t,dt,normal,fluxnonconservativeProduct);

  double lambdaMaxL = maxEigenvalue(QL,x,dx,t,dt,normal);
  double lambdaMaxR = maxEigenvalue(QR,x,dx,t,dt,normal);

  nonCriticalAssertion7( lambdaMaxL>=0.0, x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );
  nonCriticalAssertion7( lambdaMaxR>=0.0, x, dx, t, dt, normal, lambdaMaxL, lambdaMaxR );

  double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

  for (int unknown=0; unknown<unknowns; unknown++) {
    FL[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) - 0.5 * fluxnonconservativeProduct[unknown];
    FR[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) + 0.5 * fluxnonconservativeProduct[unknown];
  }
};

