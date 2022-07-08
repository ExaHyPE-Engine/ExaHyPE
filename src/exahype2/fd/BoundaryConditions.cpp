#include "BoundaryConditions.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/logging/Log.h"

#include "exahype2/fd/PatchUtils.h"


void exahype2::fd::applyBoundaryConditions(
  std::function< void(
    const double* __restrict__                   Qinside,
    double * __restrict__                        Qoutside,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   boundaryCondition,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  double                                       t,
  double                                       dt,
  int                                          numberOfGridCellsPerPatchPerAxis,
  int                                          overlap,
  int                                          unknowns,
  int                                          faceNumber,
  double* __restrict__                         Q
) {
  static tarch::logging::Log _log( "exahype2::fd" );

  auto serialisePatchIndex = [&](tarch::la::Vector<Dimensions,int> overlapCell) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {{
      result += overlapCell(d) * base;
      if (d==faceNumber % Dimensions) {{
        base *= 2*overlap;
      }}
      else {{
        base *= numberOfGridCellsPerPatchPerAxis;
      }}
    }}
    return result;
  };

  logTraceInWith4Arguments( "applyBoundaryConditions(...)", faceCentre, patchSize, numberOfGridCellsPerPatchPerAxis, faceNumber);

  tarch::la::Vector<Dimensions,double> volumeH    = exahype2::fd::getGridCellSize(patchSize, numberOfGridCellsPerPatchPerAxis);
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * patchSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * patchSize(faceNumber%Dimensions);

  dfore(volume,numberOfGridCellsPerPatchPerAxis,faceNumber % Dimensions,0) {
    tarch::la::Vector<Dimensions,int> insideGridCell  = volume;
    tarch::la::Vector<Dimensions,int> outsideGridCell = volume;
    tarch::la::Vector<Dimensions,double> x          = faceOffset + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(volume)+tarch::la::Vector<Dimensions,double>(0.5), volumeH);

    x(faceNumber%Dimensions) -= 0.5 * volumeH(faceNumber%Dimensions);

    for (int layer=0; layer<overlap; layer++){
    if (faceNumber<Dimensions) {
      insideGridCell(faceNumber % Dimensions)  = overlap-layer;
      outsideGridCell(faceNumber % Dimensions) = overlap-1-layer;
  
      int insideGridCellSerialised  = serialisePatchIndex(insideGridCell);
      int outsideGridCellSerialised = serialisePatchIndex(outsideGridCell);

      logDebug(
        "applyBoundaryConditions(...)",
        insideGridCell << "->" << outsideGridCell << " (" << insideGridCellSerialised << "->" << outsideGridCellSerialised << "): " <<
        "(" << *(Q + insideGridCellSerialised * unknowns) << ",...)^T->" <<
        "(" << *(Q + outsideGridCellSerialised * unknowns) << ",...)^T"
      );

      boundaryCondition(
        Q + insideGridCellSerialised * unknowns,
        Q + outsideGridCellSerialised * unknowns,
        x, volumeH, t, dt, faceNumber
      );
    }
    else {
      insideGridCell(faceNumber % Dimensions)  = layer+overlap-1;
      outsideGridCell(faceNumber % Dimensions) = layer+overlap;

      int insideGridCellSerialised  = serialisePatchIndex(insideGridCell);
      int outsideGridCellSerialised = serialisePatchIndex(outsideGridCell);

      logDebug(
        "applyBoundaryConditions(...)",
        insideGridCell << "->" << outsideGridCell << " (" << insideGridCellSerialised << "->" << outsideGridCellSerialised << "): " <<
        "(" << *(Q + insideGridCellSerialised * unknowns) << ",...)^T->" <<
        "(" << *(Q + outsideGridCellSerialised * unknowns) << ",...)^T"
      );

      boundaryCondition(
        Q + insideGridCellSerialised * unknowns,
        Q + outsideGridCellSerialised * unknowns,
        x, volumeH, t, dt, faceNumber
      );
    }
    }
  }

  logTraceOut( "applyBoundaryConditions(...)" );
}


