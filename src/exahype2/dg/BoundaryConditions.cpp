#include "BoundaryConditions.h"
#include "DGUtils.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/logging/Log.h"

#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"


void exahype2::dg::applyBoundaryConditions(
  std::function< void(
    const double* __restrict__                   Qinside,
    double * __restrict__                        Qoutside,
    const tarch::la::Vector<Dimensions,double>&  x,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   boundaryCondition,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  cellSize,
  double                                       t,
  double                                       dt,
  int                                          order,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  int                                          faceNumber,
  const double* __restrict__                   quadraturePoints,
  double* __restrict__                         Q
) {
  static tarch::logging::Log _log( "exahype2::dg" );

  logTraceInWith3Arguments( "applyBoundaryConditions(...)", faceCentre, cellSize, faceNumber);

//  tarch::la::Vector<Dimensions,double> volumeH    = exahype2::fv::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * cellSize(faceNumber%Dimensions);

  exahype2::enumerator::FaceAoSLexicographicEnumerator enumerator(faceNumber,order+1,1,unknowns,auxiliaryVariables);

  dfore(dof,order+1,faceNumber % Dimensions,0) {
    tarch::la::Vector<Dimensions,double> x;
    for (int d=0; d<Dimensions; d++) {
      x(d)  = faceOffset(d);
      x(d) += (d==faceNumber % Dimensions) ? 0.0 : quadraturePoints[dof(d)] * cellSize(d);
    }

    tarch::la::Vector<Dimensions,int>    insideDoF  = dof;
    tarch::la::Vector<Dimensions,int>    outsideDoF = dof;
    if (faceNumber<Dimensions) {
      insideDoF(faceNumber % Dimensions)  = 1;
      outsideDoF(faceNumber % Dimensions) = 0;
    }
    else {
      insideDoF(faceNumber % Dimensions)  = 0;
      outsideDoF(faceNumber % Dimensions) = 1;
    }

    int insideDoFSerialised  = enumerator(insideDoF,0);
    int outsideDoFSerialised = enumerator(outsideDoF,0);

    boundaryCondition(
      Q + insideDoFSerialised,
      Q + outsideDoFSerialised,
      x, t, dt, faceNumber%Dimensions
    );
  }

  logTraceOut( "applyBoundaryConditions(...)" );
}


