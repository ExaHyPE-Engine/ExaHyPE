#include "Restriction.h"
#include "Interpolation.h"
#include "Enumeration.h"

#include "peano4/utils/Loop.h"
#include "tarch/la/DynamicMatrix.h"


namespace {
  tarch::logging::Log _log( "toolbox::blockstructured" );
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


void toolbox::blockstructured::restrictCell_AoS_inject(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  dfor(fineVolume,numberOfDoFsPerAxisInPatch) {
    tarch::la::Vector<Dimensions,int>   fineVolumeWithintCoarsePatch = (fineVolume + marker.getRelativePositionWithinFatherCell() * numberOfDoFsPerAxisInPatch);
    tarch::la::Vector<Dimensions,int>   coarseVolume;

    bool restrictThisVolume = true;
    for (int d=0; d<Dimensions; d++) {
      restrictThisVolume &= (fineVolumeWithintCoarsePatch(d)%3)==1;
      coarseVolume(d) = fineVolumeWithintCoarsePatch(3)/3;
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
      tarch::la::Vector<Dimensions,int> coarseVolume,
      tarch::la::Vector<Dimensions,int> fineVolume,
      tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
      tarch::la::Vector<Dimensions,double> fineVolumeCentre,
      double coarseVolumeH,
      double fineVolumeH
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
      int targetCellSerialised = serialisePatchIndexInOverlap(
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
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   values
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

  bool mapFromInnerHalfOfHalo    = not swapInsideOutside; // mapOuterCoarseGridHaloOntoInnerFineGridHalo
  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;
  double scaleFineGridVolume     = std::pow(3.0,-static_cast<double>(Dimensions-1));
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

      tarch::la::Vector<Dimensions,double> volumeXCoarse = leftBottomCornerOfHaloCoarse
            + volumeHCoarse * tarch::la::convertScalar<double>(coarseVolume)
            + 0.5 * tarch::la::Vector<Dimensions,double>(volumeHCoarse);
      tarch::la::Vector<Dimensions,double> volumeXFine = leftBottomCornerOfHaloFine
            + volumeHFine * tarch::la::convertScalar<double>(fineVolume)
            + 0.5 * tarch::la::Vector<Dimensions,double>(volumeHFine);

      int coarseVolumeLinearised = serialisePatchIndexInOverlap(
        coarseVolume,
        numberOfDoFsPerAxisInPatch, overlap, normal
      );
      int fineVolumeLinearised = serialisePatchIndexInOverlap(
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

    const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;
    const bool pickLeftHalfOfHaloOnFineGrid  = (marker.getSelectedFaceNumber() >= Dimensions) xor swapInsideOutside;

    dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
      tarch::la::Vector<Dimensions,int>    fineVolume                = kFine;
      tarch::la::Vector<Dimensions,int>    fineVolumeAlongCoarseFace = kFine;

      bool restrictThisVolume = true;
      for (int d=0; d<Dimensions; d++) {
        if (d!=normal) {
          fineVolumeAlongCoarseFace(d)+=marker.getRelativePositionWithinFatherCell()(d)*numberOfDoFsPerAxisInPatch;

          restrictThisVolume &= (fineVolumeAlongCoarseFace(d)%3)==1;
        }
      }

      if (restrictThisVolume) {
        tarch::la::Vector<Dimensions,int>    coarseVolume = fineVolumeAlongCoarseFace/3;

        assertion(overlap==1);
      //for (int iFine=0;   iFine<overlap;   iFine+=3) {
        const int iFine=0;
        fineVolume(normal)   = pickLeftHalfOfHaloOnFineGrid ? iFine : iFine + overlap;
        coarseVolume(normal) = pickLeftHalfOfHaloOnFineGrid ? iFine : iFine + overlap;

        int coarseVolumeLinearised = serialisePatchIndexInOverlap(
          coarseVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );
        int fineVolumeLinearised = serialisePatchIndexInOverlap(
          fineVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );
        for (int j=0; j<unknowns; j++) {
          assertion3(coarseGridValues[coarseVolumeLinearised*unknowns+j]==coarseGridValues[coarseVolumeLinearised*unknowns+j], coarseVolume, fineVolume, marker.toString());
          assertion3(fineGridValues[fineVolumeLinearised*unknowns+j]==fineGridValues[fineVolumeLinearised*unknowns+j],         coarseVolume, fineVolume, marker.toString());
          coarseGridValues[coarseVolumeLinearised*unknowns+j] = fineGridValues[fineVolumeLinearised*unknowns+j];
        }
      }
    }

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
        int targetCellSerialised = serialisePatchIndexInOverlap(
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
