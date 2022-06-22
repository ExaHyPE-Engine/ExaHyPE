#include "BoundaryConditions.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/logging/Log.h"


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

  auto serialiseDoFOnFace = [&](tarch::la::Vector<Dimensions,int> overlapCell) {{
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {{
      result += overlapCell(d) * base;
      if (d==faceNumber % Dimensions) {{
        base *= 2;
      }}
      else {{
        base *= (order+1);
      }}
    }}
    return result;
  }};

  logTraceInWith3Arguments( "applyBoundaryConditions(...)", faceCentre, cellSize, faceNumber);

//  tarch::la::Vector<Dimensions,double> volumeH    = exahype2::fv::getVolumeSize(patchSize, numberOfVolumesPerAxisInPatch);
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * cellSize(faceNumber%Dimensions);

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

    int insideDoFSerialised  = serialiseDoFOnFace(insideDoF);
    int outsideDoFSerialised = serialiseDoFOnFace(outsideDoF);

    boundaryCondition(
      Q + insideDoFSerialised * (unknowns+auxiliaryVariables),
      Q + outsideDoFSerialised * (unknowns+auxiliaryVariables),
      x, t, dt, faceNumber
    );
  }

  logTraceOut( "applyBoundaryConditions(...)" );
}


