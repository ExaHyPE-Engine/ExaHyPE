#include "BoundaryConditions.h"
#include "DGUtils.h"

#include "peano4/Globals.h"
#include "peano4/Loop.h"

#include "tarch/instrumentation/Log.h"

#include "toolbox/blockstructured/Enumeration.h"


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

//  tarch::la::Vector<DIMENSIONS,double> volumeH    = exahype2::fv::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);
  const int normal = faceNumber % DIMENSIONS;
  const int stride = unknowns + auxiliaryVariables;
  tarch::la::Vector<DIMENSIONS,double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(normal) += 0.5 * cellSize(normal);

  dfore(dof,order+1,normal,0) {
    tarch::la::Vector<DIMENSIONS,double> x;
    for (int d=0; d<DIMENSIONS; d++) {
      x(d)  = faceOffset(d);
      x(d) += (d==normal) ? 0.0 : quadraturePoints[dof(d)] * cellSize(d);
    }

    tarch::la::Vector<DIMENSIONS,int>    insideDoF  = dof;
    tarch::la::Vector<DIMENSIONS,int>    outsideDoF = dof;
    if (faceNumber<DIMENSIONS) {
      insideDoF(normal)  = 1;
      outsideDoF(normal) = 0;
    }
    else {
      insideDoF(normal)  = 0;
      outsideDoF(normal) = 1;
    }

    const int insideDoFSerialised
      = toolbox::blockstructured::serialiseVoxelIndexInOverlap(insideDoF, order + 1, 1, normal) * stride;
    const int outsideDoFSerialised
      = toolbox::blockstructured::serialiseVoxelIndexInOverlap(outsideDoF, order + 1, 1, normal) * stride;

    boundaryCondition(
      Q + insideDoFSerialised,
      Q + outsideDoFSerialised,
      x, t, dt, normal
    );
  }

}

