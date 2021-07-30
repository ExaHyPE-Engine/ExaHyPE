#include "Interpolation.h"
#include "Enumeration.h"
#include "peano4/utils/Loop.h"


namespace {
  tarch::logging::Log _log( "toolbox::blockstructured" );
}


void toolbox::blockstructured::clearHalfOfHaloLayerAoS(
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


void toolbox::blockstructured::projectHaloLayers_AoS(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  std::function<void(
      tarch::la::Vector<Dimensions,int> coarseVolumeNumber,
      tarch::la::Vector<Dimensions,int> fineVolumeNumber,
      tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
      tarch::la::Vector<Dimensions,double> fineVolumeCentre,
      double courseVolumeH,
      double fineVolumeH
  )> update,
  bool mapInnerHalfOfHalo
) {
  logTraceInWith4Arguments( "projectHaloLayers_AoS(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, mapInnerHalfOfHalo );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;
  const bool pickLeftHalfOfHalo  = (marker.getSelectedFaceNumber() < Dimensions) xor mapInnerHalfOfHalo;

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

  dfore(kCoarse,numberOfDoFsPerAxisInPatch,normal,0) {
    dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
      for (int iCoarse=0; iCoarse<overlap; iCoarse++)
      for (int iFine=0;   iFine<overlap;   iFine++) {
        tarch::la::Vector<Dimensions,int> coarseVolume = kCoarse;
        tarch::la::Vector<Dimensions,int> fineVolume   = kFine;
        coarseVolume(normal) += pickLeftHalfOfHalo ? iCoarse : iCoarse+overlap;
        fineVolume(normal)   += pickLeftHalfOfHalo ? iFine   : iFine+overlap;

        tarch::la::Vector<Dimensions,double> volumeXCoarse = leftBottomCornerOfHaloCoarse
          + volumeHCoarse * tarch::la::convertScalar<double>(coarseVolume)
          + 0.5 * tarch::la::Vector<Dimensions,double>(volumeHCoarse);
        tarch::la::Vector<Dimensions,double> volumeXFine = leftBottomCornerOfHaloFine
          + volumeHFine * tarch::la::convertScalar<double>(fineVolume)
          + 0.5 * tarch::la::Vector<Dimensions,double>(volumeHFine);

        update(
          coarseVolume,
          fineVolume,
          volumeXCoarse,
          volumeXFine,
          volumeHCoarse,
          volumeHFine
        );
      }
    }
  }

  logTraceOut( "projectHaloLayers_AoS(...)" );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(marker,numberOfDoFsPerAxisInPatch,overlap,unknowns,fineGridValues,coarseGridValues,false);
  interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(marker,numberOfDoFsPerAxisInPatch,overlap,unknowns,fineGridValues,coarseGridValues,true);
}


void toolbox::blockstructured::interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues,
  bool                                      swapInsideOutside
) {
  logTraceInWith6Arguments( "interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  projectHaloLayers_AoS(
    marker,
    numberOfDoFsPerAxisInPatch,
    overlap,
    [&](
      tarch::la::Vector<Dimensions,int>    coarseVolume,
      tarch::la::Vector<Dimensions,int>    fineVolume,
      tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
      tarch::la::Vector<Dimensions,double> fineVolumeCentre,
      double coarseVolumeH,
      double fineVolumeH
    ) -> void {
      // This is where the magic happens
      bool fineGridXIsWithinSupportOfCoarseVolume = tarch::la::normMax(
        coarseVolumeCentre - fineVolumeCentre
      ) < coarseVolumeH/2.0;
      if (fineGridXIsWithinSupportOfCoarseVolume) {
        int coarseVolumeLinearised = serialisePatchIndexInOverlap(
          coarseVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );
        int fineVolumeLinearised = serialisePatchIndexInOverlap(
          fineVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );
        logDebug(
          "interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(...)",
          coarseVolume << "->" << fineVolume << " i.e. " << coarseVolumeLinearised << "->" << fineVolumeLinearised <<
          " (normal=" << normal << ",restrict=" << fineGridXIsWithinSupportOfCoarseVolume <<
          ",fine-x=" << fineVolumeCentre << ",coarse-x=" << coarseVolumeCentre <<
          ",coarse-h=" << coarseVolumeH << ",fine-h=" << fineVolumeH <<
          ",first-value-fine=" << fineGridValues[fineVolumeLinearised*unknowns] <<
          ",first-value-coarse=" << coarseGridValues[coarseVolumeLinearised*unknowns] <<
          ")"
        );
        for (int j=0; j<unknowns; j++) {
          fineGridValues[fineVolumeLinearised*unknowns+j] = coarseGridValues[coarseVolumeLinearised*unknowns+j];
        }
      }
    },
	swapInsideOutside // mapOuterCoarseGridHaloOntoInnerFineGridHalo
  );

  logTraceOut( "interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(...)" );
}


void toolbox::blockstructured::restrictOntoOuterHalfOfHaloLayer_AoS_averaging(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues,
  bool                                      swapInsideOutside
) {
  logTraceInWith6Arguments( "restrictOntoOuterHalfOfHaloLayer_AoS_averaging(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues );

  const int  normal          = marker.getSelectedFaceNumber() % Dimensions;
  double scaleFineGridVolume = std::pow(3.0,-static_cast<double>(Dimensions-1));

  logDebug( "restrictOntoOuterHalfOfHaloLayer_AoS_averaging(...)", "fineGridValues=" << fineGridValues << ",coarseGridValues=" << coarseGridValues );
  projectHaloLayers_AoS(
    marker,
    numberOfDoFsPerAxisInPatch,
    overlap,
    [&](
      tarch::la::Vector<Dimensions,int>    coarseVolume,
      tarch::la::Vector<Dimensions,int>    fineVolume,
      tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
      tarch::la::Vector<Dimensions,double> fineVolumeCentre,
      double coarseVolumeH,
      double fineVolumeH
    ) -> void {
      // This is where the magic happens
      double distance = tarch::la::normMax( coarseVolumeCentre - fineVolumeCentre );
      bool fineGridXIsWithinSupportOfCoarseVolume = distance < coarseVolumeH/2.0;
      if (fineGridXIsWithinSupportOfCoarseVolume) {
        int coarseVolumeLinearised = serialisePatchIndexInOverlap(
          coarseVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );
        int fineVolumeLinearised = serialisePatchIndexInOverlap(
          fineVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );
        for (int j=0; j<unknowns; j++) {
          assertion5(coarseGridValues[coarseVolumeLinearised*unknowns+j]==coarseGridValues[coarseVolumeLinearised*unknowns+j], coarseVolume, fineVolume, coarseVolumeCentre, fineVolumeCentre, marker.toString());
          assertion5(fineGridValues[fineVolumeLinearised*unknowns+j]==fineGridValues[fineVolumeLinearised*unknowns+j],         coarseVolume, fineVolume, coarseVolumeCentre, fineVolumeCentre, marker.toString());
          coarseGridValues[coarseVolumeLinearised*unknowns+j] += scaleFineGridVolume * fineGridValues[fineVolumeLinearised*unknowns+j];
        }
        logDebug(
          "restrictOntoOuterHalfOfHaloLayer_AoS_averaging(...)",
          fineVolume << "->" << coarseVolume << " i.e. " << fineVolumeLinearised << "->" << coarseVolumeLinearised <<
          " (normal=" << normal << ",restrict=" << fineGridXIsWithinSupportOfCoarseVolume <<
          ",fine-x=" << fineVolumeCentre << ",coarse-x=" << coarseVolumeCentre <<
          ",coarse-h=" << coarseVolumeH << ",fine-h=" << fineVolumeH << ",d=" << distance <<
          ",first-value-fine=" << fineGridValues[fineVolumeLinearised*unknowns] <<
          ",first-value-coarse=" << coarseGridValues[coarseVolumeLinearised*unknowns] <<
          ",fine-add=" << (&fineGridValues[fineVolumeLinearised*unknowns]) <<
          ",coarse-add=" << (&coarseGridValues[coarseVolumeLinearised*unknowns]) <<
          ")"
        );
      }
    },
    not swapInsideOutside // mapOuterCoarseGridHaloOntoInnerFineGridHalo
  );

  logTraceOut( "restrictOntoOuterHalfOfHaloLayer_AoS_averaging(...)" );
}


void toolbox::blockstructured::projectCells_AoS(
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

  dfor(kCoarse,numberOfDoFsPerAxisInPatch) {
    dfor(kFine,numberOfDoFsPerAxisInPatch) {
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
  }

  logTraceOut( "projectCells_AoS(...)" );
}


void toolbox::blockstructured::restrictHaloLayer_AoS_averaging(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  restrictOntoOuterHalfOfHaloLayer_AoS_averaging( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, false );
  restrictOntoOuterHalfOfHaloLayer_AoS_averaging( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, true );
}



void toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  projectCells_AoS(
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
      for (int j=0; j<unknowns; j++) {
        fineGridValues[coarseVolumeLinearised*unknowns+j] = coarseGridValues[fineVolumeLinearised*unknowns+j];
      }
    }
  );
}


void toolbox::blockstructured::restrictCell_AoS_averaging(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  double scaleFineGridVolume = std::pow(3.0,-static_cast<double>(Dimensions-1));
  projectCells_AoS(
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
      for (int j=0; j<unknowns; j++) {
        coarseGridValues[coarseVolumeLinearised*unknowns+j] += scaleFineGridVolume * fineGridValues[fineVolumeLinearised*unknowns+j];
      }
    }
  );
}
