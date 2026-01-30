#include "BoundaryConditions.h"
#include "DGUtils.h"

#include "peano4/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/logging/Log.h"

#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"


void exahype2::dg::applyBoundaryConditions(
  std::function< void(
    const double* NOALIAS                   Qinside,
    double * NOALIAS                        Qoutside,
    const tarch::la::Vector<DIMENSIONS,double>&  x,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   boundaryCondition,
  const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
  const tarch::la::Vector<DIMENSIONS,double>&  cellSize,
  double                                       t,
  double                                       dt,
  int                                          order,
  int                                          unknowns,
  int                                          auxiliaryVariables,
  int                                          faceNumber,
  const double* NOALIAS                   quadraturePoints,
  double* NOALIAS                         Q
) {
  static tarch::logging::Log _log( "exahype2::dg" );

  logTraceInWith3Arguments( "applyBoundaryConditions(...)", faceCentre, cellSize, faceNumber);

//  tarch::la::Vector<DIMENSIONS,double> volumeH    = exahype2::fv::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);
  tarch::la::Vector<DIMENSIONS,double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(faceNumber%DIMENSIONS) += 0.5 * cellSize(faceNumber%DIMENSIONS);

  exahype2::enumerator::FaceAoSLexicographicEnumerator enumerator(faceNumber,order+1,1,unknowns,auxiliaryVariables);

  dfore(dof,order+1,faceNumber % DIMENSIONS,0) {
    tarch::la::Vector<DIMENSIONS,double> x;
    for (int d=0; d<DIMENSIONS; d++) {
      x(d)  = faceOffset(d);
      x(d) += (d==faceNumber % DIMENSIONS) ? 0.0 : quadraturePoints[dof(d)] * cellSize(d);
    }

    tarch::la::Vector<DIMENSIONS,int>    insideDoF  = dof;
    tarch::la::Vector<DIMENSIONS,int>    outsideDoF = dof;
    if (faceNumber<DIMENSIONS) {
      insideDoF(faceNumber % DIMENSIONS)  = 1;
      outsideDoF(faceNumber % DIMENSIONS) = 0;
    }
    else {
      insideDoF(faceNumber % DIMENSIONS)  = 0;
      outsideDoF(faceNumber % DIMENSIONS) = 1;
    }

    int insideDoFSerialised  = enumerator(insideDoF,0);
    int outsideDoFSerialised = enumerator(outsideDoF,0);

    boundaryCondition(
      Q + insideDoFSerialised,
      Q + outsideDoFSerialised,
      x, t, dt, faceNumber%DIMENSIONS
    );
  }

  logTraceOut( "applyBoundaryConditions(...)" );
}


