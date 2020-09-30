#include "Rusanov.h"
#include "Generic.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"

#include "peano4/utils/Loop.h"

#include "exahype2/NonCriticalAssertions.h"


void exahype2::fv::splitRusanov1d(
  std::function< void(
    double                                       Q[],
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       F[]
  ) >   flux,
  std::function< double(
    double                                       Q[],
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  double QL[],
  double QR[],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       dx,
  double                                       t,
  double                                       dt,
  int                                          normal,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  double                                       FL[],
  double                                       FR[]
) {
  assertion(normal>=0);
  assertion(normal<Dimensions);

  double fluxFL[unknowns];
  double fluxFR[unknowns];
  flux(QL,x,dx,t,dt,normal,fluxFL);
  flux(QR,x,dx,t,dt,normal,fluxFR);

  double lambdaMaxL = maxEigenvalue(QL,x,dx,t,dt,normal);
  double lambdaMaxR = maxEigenvalue(QR,x,dx,t,dt,normal);
  double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

  for (int unknown=0; unknown<unknowns; unknown++) {
    FL[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
    FR[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]);
  }
};


void exahype2::fv::splitRusanov1d(
   std::function< void(
    double                                       Q[],
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       F[]
  ) >   flux,
  std::function< void(
    double                                       Q[],
    double                                       gradQ[][Dimensions],
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       BgradQ[]
  ) >   nonconservativeProduct,
  std::function< double(
    double                                       Q[],
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  double QL[],
  double QR[],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       dx,
  double                                       t,
  double                                       dt,
  int                                          normal,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  double                                       FL[],
  double                                       FR[]
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
  double lambdaMax  = std::max( lambdaMaxL, lambdaMaxR );

  for (int unknown=0; unknown<unknowns; unknown++) {
    FL[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) - 0.5 * fluxnonconservativeProduct[unknown];
    FR[unknown] = 0.5 * fluxFL[unknown] + 0.5 * fluxFR[unknown] - 0.5 * lambdaMax * (QR[unknown] - QL[unknown]) + 0.5 * fluxnonconservativeProduct[unknown];
  }
};

