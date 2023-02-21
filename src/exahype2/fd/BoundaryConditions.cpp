#include "BoundaryConditions.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "tarch/logging/Log.h"

#include "exahype2/fd/PatchUtils.h"
#include "exahype2/enumerator/FaceAoSLexicographicEnumerator.h"


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
  int                                          unknownsPlusAuxiliaryVariables,
  int                                          faceNumber,
  double* __restrict__                         Q
) {
  static tarch::logging::Log _log( "exahype2::fd" );
  logTraceInWith4Arguments( "applyBoundaryConditions(...)", faceCentre, patchSize, numberOfGridCellsPerPatchPerAxis, faceNumber);

  exahype2::enumerator::FaceAoSLexicographicEnumerator enumerator(
    faceNumber,
    numberOfGridCellsPerPatchPerAxis,
    overlap,
    unknownsPlusAuxiliaryVariables,
    0
  );

  tarch::la::Vector<Dimensions,double> volumeH    = exahype2::fd::getGridCellSize(patchSize, numberOfGridCellsPerPatchPerAxis);
  tarch::la::Vector<Dimensions,double> faceOffset = faceCentre - 0.5 * patchSize;
  faceOffset(faceNumber%Dimensions) += 0.5 * patchSize(faceNumber%Dimensions);

  dfore(volume,numberOfGridCellsPerPatchPerAxis,faceNumber % Dimensions,0) {
    tarch::la::Vector<Dimensions,int> insideGridCell  = volume;
    tarch::la::Vector<Dimensions,int> outsideGridCell = volume;
    tarch::la::Vector<Dimensions,double> x          = faceOffset + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(volume)+tarch::la::Vector<Dimensions,double>(0.5), volumeH);

    x(faceNumber%Dimensions) -= 0.5 * volumeH(faceNumber%Dimensions);

    for (int layer=0; layer<overlap; layer++) {
      if (faceNumber<Dimensions) {
        insideGridCell(faceNumber % Dimensions)  = overlap-layer;
        outsideGridCell(faceNumber % Dimensions) = overlap-1-layer;
      }
      else {
        insideGridCell(faceNumber % Dimensions)  = layer+overlap-1;
        outsideGridCell(faceNumber % Dimensions) = layer+overlap;
      }

      logDebug(
        "applyBoundaryConditions(...)",
        faceCentre << " x " << faceNumber << ": " <<
        insideGridCell << "->" << outsideGridCell << " (" << enumerator(insideGridCell,0) << "->" << enumerator(outsideGridCell,0) << "): " <<
        *(Q + enumerator(insideGridCell,0))
      );

      boundaryCondition(
        Q + enumerator(insideGridCell,0),
        Q + enumerator(outsideGridCell,0),
        x, volumeH, t, dt, faceNumber
      );
    }
  }

  logTraceOut( "applyBoundaryConditions(...)" );
}


void exahype2::fd::applySommerfeldConditions(
  std::function< double(
    const double* __restrict__                   Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  gridCellH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) >   maxEigenvalue,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  patchSize,
  double                                       dt,
  int                                          numberOfGridCellsPerPatchPerAxis,
  int                                          overlap,
  int                                          unknownsPlusAuxiliaryVariables,
  int                                          faceNumber,
  const tarch::la::Vector<Dimensions,double>&  systemOrigin,
  double* __restrict__                         Qold,
  double* __restrict__                         Qnew
) {
  static tarch::logging::Log _log( "exahype2::fd" );
  logTraceInWith4Arguments( "applySommerfeldConditions(...)", faceCentre, patchSize, numberOfGridCellsPerPatchPerAxis, faceNumber);
  logTraceOut( "applySommerfeldConditions(...)" );
}
