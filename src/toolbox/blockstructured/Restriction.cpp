#include "Restriction.h"
#include "Interpolation.h"
#include "Enumeration.h"

#include "peano4/utils/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/la/DynamicMatrix.h"

namespace {
  [[maybe_unused]] tarch::logging::Log _log( "toolbox::blockstructured" );
}

void toolbox::blockstructured::restrictCell_AoS_tensor_product(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int                                       numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int                                       unknowns,
  [[maybe_unused]] const double* __restrict__                tangentialRestrictionMatrix1d,
  [[maybe_unused]] double*                                   fineGridValues,
  [[maybe_unused]] double*                                   coarseGridValues
) {
  assertionMsg( false, "@Han, we have to implement this one" );
}

void toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                normalRestrictionMatrix1d,
  const double* __restrict__                tangentialRestrictionMatrix1d,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  restrictInnerHalfOfHaloLayer_AoS_tensor_product( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, normalRestrictionMatrix1d, tangentialRestrictionMatrix1d, fineGridValues, coarseGridValues, false );
  restrictInnerHalfOfHaloLayer_AoS_tensor_product( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, normalRestrictionMatrix1d, tangentialRestrictionMatrix1d, fineGridValues, coarseGridValues, true );
}

void toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  const double* __restrict__                normalRestrictionMatrix1d,
  const double* __restrict__                tangentialRestrictionMatrix1d,
  double*                                   fineGridValues,
  double*                                   coarseGridValues,
  bool                                      swapInsideOutside
) {
  logTraceInWith3Arguments( "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap );

  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  dfore(kCoarse, numberOfDoFsPerAxisInPatch, normal, 0) {
    for (int iCoarse=0; iCoarse<overlap; iCoarse++) {
      tarch::la::Vector<Dimensions,int> dest = kCoarse;
      dest(normal) = ((marker.getSelectedFaceNumber() < Dimensions) xor swapInsideOutside) ? iCoarse + overlap: overlap - iCoarse - 1;

      int destLinearised = serialiseVoxelIndexInOverlap(
        dest,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );

      logDebug( "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)", "coarse volume/dof " << dest << " prior to update: " << coarseGridValues[destLinearised*5+0] );
      assertion(  coarseGridValues[destLinearised*5+0]==coarseGridValues[destLinearised*5+0] );

      dfore(kFine, numberOfDoFsPerAxisInPatch, normal, 0) {
        for (int iFine = 0; iFine < 2 * overlap; iFine++) {
          tarch::la::Vector<Dimensions,int> src  = kFine;
          src(normal) = ((marker.getSelectedFaceNumber() < Dimensions) xor swapInsideOutside) ? iFine : 2 * overlap - iFine - 1;
          int srcLinearised = serialiseVoxelIndexInOverlap(
            src,
            numberOfDoFsPerAxisInPatch, overlap, normal
          );

          double weight = 1.0;
          for (int d=0; d<Dimensions; d++) {
            if (d==normal) {
              int col   = iFine;
              int row   = iCoarse;

              assertion4(col>=0,row,col,src,dest);
              assertion4(row>=0,row,col,src,dest);

              assertion4(col<2*overlap,row,col,src,dest);
              assertion4(row<  overlap,row,col,src,dest);

              int index = col + row * 2 * overlap;
              weight *= normalRestrictionMatrix1d[ index ];
              logDebug( "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)", "use normal matrix entry " << row << ", " << col << ", i.e. index " << index << ": " << normalRestrictionMatrix1d[ index ]);
            } else {
              assertion( marker.getRelativePositionWithinFatherCell()(d)>=0 );
              assertion( marker.getRelativePositionWithinFatherCell()(d)<3 );

              assertion( src(d)>=0 );
              int col   = src(d)+ marker.getRelativePositionWithinFatherCell()(d) * numberOfDoFsPerAxisInPatch;
              int row   = dest(d);
              assertion5(col>=0,row,col,src,dest, marker.getRelativePositionWithinFatherCell());
              assertion5(row>=0,row,col,src,dest, marker.getRelativePositionWithinFatherCell());
              assertion5(col<3*numberOfDoFsPerAxisInPatch,row,col,src,dest, marker.getRelativePositionWithinFatherCell());
              assertion5(row<  numberOfDoFsPerAxisInPatch,row,col,src,dest, marker.getRelativePositionWithinFatherCell());
              int index = col + row * 3 * numberOfDoFsPerAxisInPatch;
              weight *= tangentialRestrictionMatrix1d[ index ];
              logDebug( "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)", "use tangential matrix entry " << row << ", " << row << ", i.e. index " << index << ": " << tangentialRestrictionMatrix1d[ index ]);
            }
          }

          assertion2(weight==weight, srcLinearised, destLinearised);

          for (int unknown=0; unknown<unknowns; unknown++) {
            assertion2( fineGridValues[srcLinearised*unknowns+unknown]    == fineGridValues[srcLinearised*unknowns+unknown],    srcLinearised, destLinearised );
            coarseGridValues[destLinearised*unknowns+unknown] += weight * fineGridValues[srcLinearised*unknowns+unknown];
            assertion2( coarseGridValues[destLinearised*unknowns+unknown] == coarseGridValues[destLinearised*unknowns+unknown], srcLinearised, destLinearised );
          }

          logDebug(
            "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)",
            "add dof " << src  << " (" << srcLinearised << ") to "
                       << dest << " (" << destLinearised << ") with weight " << weight << " for marker " << marker.toString() <<
            ": " << coarseGridValues[destLinearised*unknowns+0] << "<-" << fineGridValues[srcLinearised*unknowns+0] );
        }
      }
      logDebug( "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)", "update coarse volume/dof " << dest << ": " << coarseGridValues[destLinearised*5+0] );
    }
  }

  logTraceOut( "restrictInnerHalfOfHaloLayer_AoS_tensor_product(...)" );
}

void toolbox::blockstructured::restrictHaloLayer_AoS_inject(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  restrictInnerHalfOfHaloLayer_AoS_inject( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, false );
  restrictInnerHalfOfHaloLayer_AoS_inject( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, true );
}

void toolbox::blockstructured::restrictCellIntoOverlappingCell_inject(
  int                                       numberOfDoFsPerAxisInSourcePatch,
  int                                       numberOfDoFsPerAxisInDestinationPatch,
  int                                       unknowns,
  double*                                   sourceValues,
  double*                                   destinationValues
) {
  // how many fine grid cells are mapped onto one destination cell
  double sourceToDestinationRatio = static_cast<double>(numberOfDoFsPerAxisInSourcePatch) / static_cast<double>(numberOfDoFsPerAxisInDestinationPatch);
  dfor( destinationVolume, numberOfDoFsPerAxisInDestinationPatch ) {
    const int baseIndexDestination = peano4::utils::dLinearised(destinationVolume,numberOfDoFsPerAxisInDestinationPatch) * unknowns;

    tarch::la::Vector<Dimensions,int> sourceVolume;
    for (int d=0; d<Dimensions; d++) {
      sourceVolume(d) = std::floor(
        static_cast<double>(destinationVolume(d)) * sourceToDestinationRatio + 0.5 * sourceToDestinationRatio
      );
    }

    const int baseIndexSource = peano4::utils::dLinearised(sourceVolume,numberOfDoFsPerAxisInSourcePatch) * unknowns;
    for (int unknown=0; unknown<unknowns; unknown++) {
      destinationValues[baseIndexDestination+unknown] = sourceValues[baseIndexSource+unknown];
    }
  }
}

void toolbox::blockstructured::restrictCellIntoOverlappingCell_inject_and_average(
  int     numberOfDoFsPerAxisInSourcePatch,
  int     numberOfDoFsPerAxisInDestinationPatch,
  int     unknowns,
  double* sourceValues,
  double* destinationValues,
  double  weightOfInjectedValue
) {
  assertion1( weightOfInjectedValue>=0.0, weightOfInjectedValue );
  assertion1( weightOfInjectedValue<=1.0, weightOfInjectedValue );

  // how many fine grid cells are mapped onto one destination cell
  double sourceToDestinationRatio = static_cast<double>(numberOfDoFsPerAxisInSourcePatch) / static_cast<double>(numberOfDoFsPerAxisInDestinationPatch);
  dfor( destinationVolume, numberOfDoFsPerAxisInDestinationPatch ) {
    const int baseIndexDestination = peano4::utils::dLinearised(destinationVolume,numberOfDoFsPerAxisInDestinationPatch) * unknowns;

    tarch::la::Vector<Dimensions,int> sourceVolume;
    for (int d=0; d<Dimensions; d++) {
      sourceVolume(d) = std::floor(
        static_cast<double>(destinationVolume(d)) * sourceToDestinationRatio + 0.5 * sourceToDestinationRatio
      );
    }

    const int baseIndexSource = peano4::utils::dLinearised(sourceVolume,numberOfDoFsPerAxisInSourcePatch) * unknowns;
    for (int unknown=0; unknown<unknowns; unknown++) {
      destinationValues[baseIndexDestination+unknown] =
          weightOfInjectedValue       * sourceValues[baseIndexSource+unknown]
        + (1.0-weightOfInjectedValue) * destinationValues[baseIndexDestination+unknown];
    }
  }
}

void toolbox::blockstructured::restrictCell_AoS_inject(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  dfor(fineVolume,numberOfDoFsPerAxisInPatch) {
    tarch::la::Vector<Dimensions, int> fineVolumeWithintCoarsePatch = (fineVolume + marker.getRelativePositionWithinFatherCell() * numberOfDoFsPerAxisInPatch);
    tarch::la::Vector<Dimensions, int> coarseVolume;

    bool restrictThisVolume = true;
    for (int d = 0; d < Dimensions; d++) {
      restrictThisVolume &= (fineVolumeWithintCoarsePatch(d) % 3) == 1;
      coarseVolume(d) = fineVolumeWithintCoarsePatch(3) / 3;
    }

    if (restrictThisVolume) {
      int coarseVolumeLinearised = peano4::utils::dLinearised( coarseVolume, numberOfDoFsPerAxisInPatch );
      int fineVolumeLinearised   = peano4::utils::dLinearised( fineVolume, numberOfDoFsPerAxisInPatch );
      for (int j=0; j<unknowns; j++) {
        assertion3(coarseGridValues[coarseVolumeLinearised*unknowns+j]==coarseGridValues[coarseVolumeLinearised*unknowns+j], coarseVolume, fineVolume, marker.toString());
        assertion3(fineGridValues[fineVolumeLinearised*unknowns+j]==fineGridValues[fineVolumeLinearised*unknowns+j],         coarseVolume, fineVolume, marker.toString());
        coarseGridValues[coarseVolumeLinearised*unknowns+j] = fineGridValues[fineVolumeLinearised*unknowns+j];
      }
    }
  }
}

void toolbox::blockstructured::restrictHaloLayer_AoS_averaging(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  restrictInnerHalfOfHaloLayer_AoS_averaging( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, false );
  restrictInnerHalfOfHaloLayer_AoS_averaging( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, true );
}

void toolbox::blockstructured::restrictCell_AoS_averaging(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  double scaleFineGridVolume = std::pow(3.0,-static_cast<double>(Dimensions));
  internal::projectCells_AoS(
    marker,
    numberOfDoFsPerAxisInPatch,
    [&](
      [[maybe_unused]] tarch::la::Vector<Dimensions, int> coarseVolume,
      [[maybe_unused]] tarch::la::Vector<Dimensions, int> fineVolume,
      [[maybe_unused]] tarch::la::Vector<Dimensions, double> coarseVolumeCentre,
      [[maybe_unused]] tarch::la::Vector<Dimensions, double> fineVolumeCentre,
      [[maybe_unused]] double coarseVolumeH,
      [[maybe_unused]] double fineVolumeH
    ) -> void {
      int coarseVolumeLinearised = peano4::utils::dLinearised( coarseVolume, numberOfDoFsPerAxisInPatch );
      int fineVolumeLinearised   = peano4::utils::dLinearised( fineVolume, numberOfDoFsPerAxisInPatch );
      logDebug( "restrictCell_AoS_averaging(...)", fineVolume << " -> " << coarseVolume );
      for (int j=0; j<unknowns; j++) {
        coarseGridValues[coarseVolumeLinearised*unknowns+j] += scaleFineGridVolume * fineGridValues[fineVolumeLinearised*unknowns+j];
      }
    }
  );
}

void toolbox::blockstructured::clearHaloLayerAoS(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   value
) {
  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  // clear fine grid data structure
  dfore(k,numberOfDoFsPerAxisInPatch,normal,0) {
    for (int i=0; i<overlap*2; i++) {
      tarch::la::Vector<Dimensions,int> targetCell = k;
      targetCell(normal)  = i;
      int targetCellSerialised = serialiseVoxelIndexInOverlap(
        targetCell,
        numberOfDoFsPerAxisInPatch,
        overlap,
        normal
      );
      assertion(targetCellSerialised>=0);
      for (int j=0; j<unknowns; j++) {
        value[targetCellSerialised*unknowns+j] = 0.0;
      }
    }
  }
}

void toolbox::blockstructured::clearCell(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int                                       numberOfDoFsPerAxisInPatch,
  [[maybe_unused]] int                                       unknowns,
  [[maybe_unused]] double*                                   values
) {
  #if Dimensions==3
  for (int i=0; i<numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*unknowns; i++) {
  #else
  for (int i=0; i<numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*unknowns; i++) {
  #endif
    values[i] = 0.0;
  }
}

void toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_averaging(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues,
  bool                                      swapInsideOutside
) {
  logTraceInWith4Arguments( "restrictInnerHalfOfHaloLayer_AoS_averaging(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  assertion1( overlap==1 or overlap%3==0, overlap );

  bool mapFromInnerHalfOfHalo    = not swapInsideOutside; // mapOuterCoarseGridHaloOntoInnerFineGridHalo
  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;
  double scaleFineGridVolume     = std::pow(3.0,-static_cast<double>(Dimensions-1)) / std::min(3.0,static_cast<double>(overlap));
  const bool pickLeftHalfOfHalo  = (marker.getSelectedFaceNumber() < Dimensions) xor mapFromInnerHalfOfHalo;

  const double volumeHCoarse = marker.h()(0) / static_cast<double>(numberOfDoFsPerAxisInPatch) * 3.0;
  const double volumeHFine   = marker.h()(0) / static_cast<double>(numberOfDoFsPerAxisInPatch);

  tarch::la::Vector<Dimensions,double> leftBottomCornerOfHaloFine   = marker.x();
  tarch::la::Vector<Dimensions,double> leftBottomCornerOfHaloCoarse = marker.x();
  for (int d=0; d<Dimensions; d++) {
    if (d==normal) {
      leftBottomCornerOfHaloFine(d)   -= static_cast<double>(overlap) * volumeHFine;
      leftBottomCornerOfHaloCoarse(d) -= static_cast<double>(overlap) * volumeHCoarse;
    }
    else {
      leftBottomCornerOfHaloFine(d)   = marker.x()(d)-marker.h()(d)/2.0;
      leftBottomCornerOfHaloCoarse(d) = leftBottomCornerOfHaloFine(d)-marker.getRelativePositionWithinFatherCell()(d)*marker.h()(d);
    }
  }

  dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
    for (int iFine=0;   iFine<overlap;   iFine++) {
      tarch::la::Vector<Dimensions,int> fineVolume   = kFine;
      fineVolume(normal) += pickLeftHalfOfHalo ? iFine : iFine + overlap;

      tarch::la::Vector<Dimensions,int> coarseVolume = fineVolume;
      for (int d=0; d<Dimensions; d++) {
        if (d!=normal) {
          coarseVolume(d) += marker.getRelativePositionWithinFatherCell()(d) * numberOfDoFsPerAxisInPatch;
        }
        else {
          coarseVolume(d) = pickLeftHalfOfHalo ? fineVolume(d) : fineVolume(d) + overlap * 3;
        }
      }
      coarseVolume /= 3;

      assertion3( coarseVolume(0)>=0, kFine, iFine, marker );
      assertion3( coarseVolume(1)>=0, kFine, iFine, marker );
      assertion3( coarseVolume(0)<numberOfDoFsPerAxisInPatch, kFine, iFine, marker );
      assertion3( coarseVolume(1)<numberOfDoFsPerAxisInPatch, kFine, iFine, marker );

      [[maybe_unused]] tarch::la::Vector<Dimensions, double> volumeXCoarse = leftBottomCornerOfHaloCoarse
            + volumeHCoarse * tarch::la::convertScalar<double>(coarseVolume)
            + 0.5 * tarch::la::Vector<Dimensions, double>(volumeHCoarse);
      [[maybe_unused]] tarch::la::Vector<Dimensions, double> volumeXFine = leftBottomCornerOfHaloFine
            + volumeHFine * tarch::la::convertScalar<double>(fineVolume)
            + 0.5 * tarch::la::Vector<Dimensions, double>(volumeHFine);

      int coarseVolumeLinearised = serialiseVoxelIndexInOverlap(
        coarseVolume,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );
      int fineVolumeLinearised = serialiseVoxelIndexInOverlap(
        fineVolume,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );
      for (int j=0; j<unknowns; j++) {
        coarseGridValues[coarseVolumeLinearised*unknowns+j] += scaleFineGridVolume * fineGridValues[fineVolumeLinearised*unknowns+j];
      }
    }
  }

  logTraceOut( "restrictInnerHalfOfHaloLayer_AoS_averaging(...)" );
}

void toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_inject(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues,
  bool                                      swapInsideOutside
) {
    logTraceInWith6Arguments( "restrictInnerHalfOfHaloLayer_AoS_inject(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues );

    assertion1( overlap==1 or overlap%3==1, overlap );

    const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;
    const bool pickLeftHalfOfHaloOnFineGrid  = (marker.getSelectedFaceNumber() >= Dimensions) xor swapInsideOutside;

    dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
      tarch::la::Vector<Dimensions,int>    fineVolume                = kFine;
      tarch::la::Vector<Dimensions,int>    fineVolumeAlongCoarseFace = kFine;

      for (int iFine=0; iFine<overlap; iFine++) {
        fineVolume(normal) = iFine;

        bool restrictThisVolume = true;
        for (int d=0; d<Dimensions; d++) {
          if (d==normal) {
            restrictThisVolume &= (overlap==1 or fineVolumeAlongCoarseFace(d)%3==1);
          }
          else {
            fineVolumeAlongCoarseFace(d)+=marker.getRelativePositionWithinFatherCell()(d)*numberOfDoFsPerAxisInPatch;
            restrictThisVolume &= (fineVolumeAlongCoarseFace(d)%3)==1;
          }
        }

        if (restrictThisVolume) {
          tarch::la::Vector<Dimensions,int>    coarseVolume = fineVolumeAlongCoarseFace/3;

          fineVolume(normal)   = pickLeftHalfOfHaloOnFineGrid ? iFine : iFine + overlap;
          coarseVolume(normal) = pickLeftHalfOfHaloOnFineGrid ? iFine : iFine + overlap;

          int coarseVolumeLinearised = serialiseVoxelIndexInOverlap(
            coarseVolume,
            numberOfDoFsPerAxisInPatch, overlap, normal
            );
          int fineVolumeLinearised = serialiseVoxelIndexInOverlap(
            fineVolume,
            numberOfDoFsPerAxisInPatch, overlap, normal
            );
          for (int j=0; j<unknowns; j++) {
            assertion3(coarseGridValues[coarseVolumeLinearised*unknowns+j]==coarseGridValues[coarseVolumeLinearised*unknowns+j], coarseVolume, fineVolume, marker.toString());
            assertion3(fineGridValues[fineVolumeLinearised*unknowns+j]==fineGridValues[fineVolumeLinearised*unknowns+j],         coarseVolume, fineVolume, marker.toString());
            coarseGridValues[coarseVolumeLinearised*unknowns+j] = fineGridValues[fineVolumeLinearised*unknowns+j];
          }
        } // end of restrictThisVolume
      } // for iFine loop, i.e. loop over halo layer depth
    } // dfore loop, i.e. loop over submanifold

    logTraceOut( "restrictInnerHalfOfHaloLayer_AoS_restrict(...)" );
}

void toolbox::blockstructured::internal::clearHalfOfHaloLayerAoS(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  bool                                      clearInnerPart,
  double*                                   value
) {
    const int normal = marker.getSelectedFaceNumber() % Dimensions;
    const bool left  = (marker.getSelectedFaceNumber() < Dimensions) xor clearInnerPart;

    // clear fine grid data structure
    dfore(k,numberOfDoFsPerAxisInPatch,normal,0) {
      for (int i=0; i<overlap; i++) {
        tarch::la::Vector<Dimensions,int> targetCell = k;
        targetCell(normal)  = left ? i : i + overlap;
        int targetCellSerialised = serialiseVoxelIndexInOverlap(
          targetCell,
          numberOfDoFsPerAxisInPatch,
          overlap,
          normal
        );
        assertion(targetCellSerialised>=0);
        for (int j=0; j<unknowns; j++) {
          value[targetCellSerialised*unknowns+j] = 0.0;
        }
      }
    }
}

void toolbox::blockstructured::internal::projectCells_AoS(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  std::function<void(
    tarch::la::Vector<Dimensions,int> coarseVolume,
    tarch::la::Vector<Dimensions,int> fineVolume,
    tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
    tarch::la::Vector<Dimensions,double> fineVolumeCentre,
    double coarseVolumeH,
    double fineVolumeH
  )> update
) {
  logTraceInWith2Arguments( "projectCells_AoS(...)", marker.toString(), numberOfDoFsPerAxisInPatch );

    const double volumeHCoarse = marker.h()(0) / static_cast<double>(numberOfDoFsPerAxisInPatch) * 3.0;
    const double volumeHFine   = marker.h()(0) / static_cast<double>(numberOfDoFsPerAxisInPatch);

    tarch::la::Vector<Dimensions,double> leftBottomOfFineCell   = marker.getOffset();
    tarch::la::Vector<Dimensions,double> leftBottomOfCoarseCell = marker.getOffset() - tarch::la::multiplyComponents( marker.h(), tarch::la::convertScalar<double>(marker.getRelativePositionWithinFatherCell()) );

    dfor(kFine,numberOfDoFsPerAxisInPatch) {
      tarch::la::Vector<Dimensions,int> kCoarse = (kFine + numberOfDoFsPerAxisInPatch * marker.getRelativePositionWithinFatherCell()) / 3;

      tarch::la::Vector<Dimensions,double> volumeXCoarse = leftBottomOfCoarseCell
            + volumeHCoarse * tarch::la::convertScalar<double>(kCoarse)
            + 0.5 * tarch::la::Vector<Dimensions,double>(volumeHCoarse);
      tarch::la::Vector<Dimensions,double> volumeXFine = leftBottomOfFineCell
            + volumeHFine * tarch::la::convertScalar<double>(kFine)
            + 0.5 * tarch::la::Vector<Dimensions,double>(volumeHFine);

      update(
        kCoarse,
        kFine,
        volumeXCoarse,
        volumeXFine,
        volumeHCoarse,
        volumeHFine
      );
    }

    logTraceOut( "projectCells_AoS(...)" );
}
