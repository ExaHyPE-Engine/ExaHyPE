#include "Interpolation.h"
#include "Enumeration.h"

#include "peano4/utils/Loop.h"
#include "tarch/la/DynamicMatrix.h"


namespace {
  tarch::logging::Log _log( "toolbox::blockstructured" );
}


tarch::la::DynamicMatrix  toolbox::blockstructured::internal::createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch, int normal) {
  logTraceInWith2Arguments( "createLinearInterpolationMatrix(...)", numberOfDoFsPerAxisInPatch, normal );
  tarch::la::DynamicMatrix P1d(3,3,{
      {1.0/3.0, 2.0/3.0,     0.0},
      {    0.0, 3.0/3.0,     0.0},
      {    0.0, 2.0/3.0, 1.0/3.0}
  });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  /*
   * See comments on missing diagonal element
   * ========================================
    P1d(0,0) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(0,1) = -1.0/3.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  2.0/3.0 + 1.0/3.0 * 2.0;
    P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) = -1.0/3.0;
  */

  P1d(0,0) =  1.0;
  P1d(0,1) =  0.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  1.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) =  0.0;

  logDebug( "createLinearInterpolationMatrix(...)", "1d matrix: " << P1d.toString() );

  int pattern = 0;
  switch (normal) {
    case 0:
      pattern = 1;
      break;
    case 1:
      pattern = numberOfDoFsPerAxisInPatch;
      break;
    case 2:
      pattern = numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch;
      break;
  }

  #if Dimensions==3
    tarch::la::DynamicMatrix P( P1d, P1d, false);
    P.insertEmptyColumns(pattern,pattern,pattern);
    P.replicateRows( pattern, 2, pattern, false );
    logTraceOutWith2Arguments( "createLinearInterpolationMatrix(...)", P1d.toString(), P.toString() );
    return P;
  #else
    P1d.insertEmptyColumns(pattern,pattern,pattern);
    logDebug( "createLinearInterpolationMatrix(...)", "matrix with extended columns: " << P1d.toString() );
    P1d.replicateRows( pattern, 2, pattern, false );
    logTraceOutWith1Argument( "createLinearInterpolationMatrix(...)", P1d.toString() );
    return P1d;
  #endif
}


tarch::la::DynamicMatrix  toolbox::blockstructured::internal::createLinearInterpolationMatrix(int numberOfDoFsPerAxisInPatch) {
  logTraceInWith1Argument( "createLinearInterpolationMatrix(...)", numberOfDoFsPerAxisInPatch );
  tarch::la::DynamicMatrix P1d(3,3,{
      {1.0/3.0, 2.0/3.0,     0.0},
      {    0.0, 3.0/3.0,     0.0},
      {    0.0, 2.0/3.0, 1.0/3.0}
  });
  P1d.replicateRows( 3, numberOfDoFsPerAxisInPatch, 1, true );
  P1d.removeColumn(0);
  P1d.removeColumn(numberOfDoFsPerAxisInPatch);

  P1d(0,0) =  1.0;
  P1d(0,1) =  0.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-1) =  1.0;
  P1d(numberOfDoFsPerAxisInPatch*3-1,numberOfDoFsPerAxisInPatch-2) =  0.0;

  logDebug( "createLinearInterpolationMatrix(...)", "1d interpolation matrix: " << P1d.toString(true) );

  tarch::la::DynamicMatrix P2d( P1d, P1d, false);
  logDebug( "createLinearInterpolationMatrix(...)", "2d interpolation matrix: " << P2d.toString(true) );

  logTraceOut( "createLinearInterpolationMatrix(...)" );
  #if Dimensions==3
  return tarch::la::DynamicMatrix( P2d, P1d, false);
  #else
  return P2d;
  #endif
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


void toolbox::blockstructured::internal::projectInterpolatedFineCellsOnHaloLayer_AoS(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  std::function<void(
    tarch::la::Vector<Dimensions,int> coarseVolume,
    tarch::la::Vector<Dimensions,int> fineVolume,
/*
    tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
    tarch::la::Vector<Dimensions,double> fineVolumeCentre,
*/
    double coarseVolumeH,
    double fineVolumeH
  )> update
) {
  logTraceInWith3Arguments( "projectInterpolatedFineCellsOnHaloLayer_AoS(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap );

  const int    normal        = marker.getSelectedFaceNumber() % Dimensions;
  const double volumeHCoarse = marker.h()(0) / static_cast<double>(numberOfDoFsPerAxisInPatch) * 3.0;
  const double volumeHFine   = marker.h()(0) / static_cast<double>(numberOfDoFsPerAxisInPatch);

  tarch::la::Vector<Dimensions,double> leftBottomCornerOfHaloFine   = marker.x();
  for (int d=0; d<Dimensions; d++) {
    if (d==normal) {
      leftBottomCornerOfHaloFine(d)   -= static_cast<double>(overlap) * volumeHFine;
    }
    else {
      leftBottomCornerOfHaloFine(d)   = marker.x()(d)-marker.h()(d)/2.0;
    }
  }

  dfore(kFine,numberOfDoFsPerAxisInPatch,normal,0) {
    for (int iFine=0;   iFine<overlap*2;   iFine++) {
      tarch::la::Vector<Dimensions,int> fineVolume   = kFine;
      fineVolume(normal)                            += iFine;

      tarch::la::Vector<Dimensions,int> coarseVolume = marker.getRelativePositionWithinFatherCell() * numberOfDoFsPerAxisInPatch;
      coarseVolume(normal) -= overlap;
      coarseVolume         += fineVolume;
      // coarseVolume          = coarseVolume / 3;

      update(
        coarseVolume,
        fineVolume,
//        volumeXCoarse,
//        volumeXFine,
        volumeHCoarse,
        volumeHFine
      );
    }
  }

  logTraceOut( "projectInterpolatedFineCellsOnHaloLayer_AoS(...)" );
}


void toolbox::blockstructured::internal::projectCoarseCellsOnHaloLayer_AoS(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  std::function<void(
    tarch::la::Vector<Dimensions,int> coarseVolume,
    tarch::la::Vector<Dimensions,int> fineVolume,
/*
    tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
    tarch::la::Vector<Dimensions,double> fineVolumeCentre,
*/
    double coarseVolumeH,
    double fineVolumeH
  )> update
) {
  logTraceInWith3Arguments( "projectCoarseCellsOnHaloLayer_AoS(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap );

  projectInterpolatedFineCellsOnHaloLayer_AoS(
    marker,
	numberOfDoFsPerAxisInPatch,
	overlap,
	[&](
      tarch::la::Vector<Dimensions,int> coarseVolume,
      tarch::la::Vector<Dimensions,int> fineVolume,
//		    tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
//		    tarch::la::Vector<Dimensions,double> fineVolumeCentre,
      double coarseVolumeH,
      double fineVolumeH
    )->void {
      coarseVolume /= 3;
      update(coarseVolume, fineVolume, coarseVolumeH, fineVolumeH);
    }
  );

  logTraceOut( "projectCoarseCellsOnHaloLayer_AoS(...)" );
}


void toolbox::blockstructured::internal::projectHaloLayers_AoS(
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
  logTraceInWith6Arguments( "interpolateHaloLayer_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues );

  const int  normal              = marker.getSelectedFaceNumber() % Dimensions;

  for (int swapInsideOutside=0; swapInsideOutside<2; swapInsideOutside++)
  internal::projectHaloLayers_AoS(
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
          "interpolateHaloLayer_AoS_piecewise_constant(...)",
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
    swapInsideOutside==0
  );

  logTraceOut( "interpolateHaloLayer_AoS_piecewise_constant(...)" );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridFaceValues,
  double*                                   coarseGridFaceValues,
  double*                                   coarseGridCellValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  int normal = marker.getSelectedFaceNumber() % Dimensions;
  if (
    marker.getRelativePositionWithinFatherCell()(normal)==0
    or
    marker.getRelativePositionWithinFatherCell()(normal)==3
  ) {
    interpolateHaloLayer_AoS_piecewise_constant(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      fineGridFaceValues,
      coarseGridFaceValues
    );
  }
  else {
    internal::projectCoarseCellsOnHaloLayer_AoS(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      [&](
        tarch::la::Vector<Dimensions,int> coarseVolume,
        tarch::la::Vector<Dimensions,int> fineVolume,
/*
        tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
        tarch::la::Vector<Dimensions,double> fineVolumeCentre,
*/
        double coarseVolumeH,
        double fineVolumeH
      )->void {
        int coarseVolumeLinearised = peano4::utils::dLinearised(
          coarseVolume,
          numberOfDoFsPerAxisInPatch
          );
        int fineVolumeLinearised = serialisePatchIndexInOverlap(
          fineVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
          );

        for (int j=0; j<unknowns; j++) {
          fineGridFaceValues[fineVolumeLinearised*unknowns+j] = coarseGridCellValues[coarseVolumeLinearised*unknowns+j];
        }
      }
    );
  }

  logTraceOut( "interpolateHaloLayer_AoS_piecewise_constant(...)" );
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


void toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  logTraceInWith3Arguments("interpolateCell_AoS_piecewise_constant(...)", marker.toString(), numberOfDoFsPerAxisInPatch, unknowns );
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
      for (int j=0; j<unknowns; j++) {
        fineGridValues[fineVolumeLinearised*unknowns+j] = coarseGridValues[coarseVolumeLinearised*unknowns+j];
      }
      logDebug( "interpolateCell_AoS_piecewise_constant(...)", coarseVolume << "->" << fineVolume << " (" << fineGridValues[coarseVolumeLinearised*unknowns] << ", " << fineGridValues[coarseVolumeLinearised*unknowns+4] << ")");
    }
  );
  logTraceOut("interpolateCell_AoS_piecewise_constant(...)");
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
  internal::projectHaloLayers_AoS(
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


void toolbox::blockstructured::restrictCell_AoS_averaging(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  double scaleFineGridVolume = std::pow(3.0,-static_cast<double>(Dimensions-1));
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
      for (int j=0; j<unknowns; j++) {
        coarseGridValues[coarseVolumeLinearised*unknowns+j] += scaleFineGridVolume * fineGridValues[fineVolumeLinearised*unknowns+j];
      }
    }
  );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_linear(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  assertion(overlap==1);

  const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;

  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...)", numberOfDoFsPerAxisInPatch, overlap, unknowns, normal );

  #if Dimensions==2
  if ( normal==0 ) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,0) );
    const int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(1);
    const int firstRowInP    = matrixRowBlock * numberOfDoFsPerAxisInPatch * 2;
    logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...) - a", matrixRowBlock, firstRowInP, P.toString(), P.vectorToString(coarseGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      firstRowInP
    );
    logTraceOutWith1Argument( "interpolateHaloLayer_AoS_linear(...) - a", P.vectorToString(fineGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
  }
  else if (normal==1) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,1) );
    const int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(0);
    const int firstRowInP    = matrixRowBlock * numberOfDoFsPerAxisInPatch * 2;
    logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...) - b", matrixRowBlock, firstRowInP, P.toString(), P.vectorToString(coarseGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      firstRowInP
    );
    logTraceOutWith1Argument( "interpolateHaloLayer_AoS_linear(...) - b", P.vectorToString(fineGridValues,numberOfDoFsPerAxisInPatch*2*unknowns) );
  }
  #elif Dimensions==3
  if ( normal==0 ) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,0) );
    int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(1) + marker.getRelativePositionWithinFatherCell()(2)*3;
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      matrixRowBlock * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2
    );
  }
  else if (normal==1) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,1) );
    int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(0) + marker.getRelativePositionWithinFatherCell()(2)*3;
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      matrixRowBlock * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2
    );
  }
  else if (normal==2) {
    tarch::la::DynamicMatrix P( internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch,2) );
    int matrixRowBlock = marker.getRelativePositionWithinFatherCell()(0) + marker.getRelativePositionWithinFatherCell()(1)*3;
    P.batchedMultiplyAoS(
      fineGridValues, // image
      coarseGridValues,  // preimage
      unknowns,          // batch size, i.e. how often to apply it in one AoS rush
      numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*2, // result size, i.e. size of image
      matrixRowBlock * numberOfDoFsPerAxisInPatch * numberOfDoFsPerAxisInPatch * 2
    );
  }
  #endif

  logTraceOut( "interpolateHaloLayer_AoS_linear(...)")
}


void toolbox::blockstructured::interpolateCell_AoS_linear(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  tarch::la::DynamicMatrix Project = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch);

  #if Dimensions==3
  tarch::la::DynamicMatrix Pick = tarch::la::DynamicMatrix::id( numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  int interleaveX =2*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveX, numberOfDoFsPerAxisInPatch, numberOfDoFsPerAxisInPatch);
  int interleaveY =2*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveY, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch);
  int interleaveZ =2*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveZ, 9*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, 9*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch);
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(0)*1*numberOfDoFsPerAxisInPatch );
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(1)*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(2)*9*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  #else
  tarch::la::DynamicMatrix Pick = tarch::la::DynamicMatrix::id( numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  int interleaveX = 2*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveX, numberOfDoFsPerAxisInPatch, numberOfDoFsPerAxisInPatch);
  int interleaveY =2*numberOfDoFsPerAxisInPatch*3*numberOfDoFsPerAxisInPatch;
  Pick.insertEmptyColumns( interleaveY, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, 3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch);
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(0)*1*numberOfDoFsPerAxisInPatch );
  Pick.shiftColumnsRight( marker.getRelativePositionWithinFatherCell()(1)*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch );
  #endif

  tarch::la::DynamicMatrix PickTimesProject = Pick * Project;

/*
  std::cout << "marker=" << marker.toString() << std::endl;
  std::cout << "Pick=" << Pick.toString(true) << std::endl;
  std::cout << "Project=" << Project.toString(true) << std::endl;
  std::cout << "PickTimesProject=" << PickTimesProject.toString(true) << std::endl;
*/

  #if Dimensions==2
  PickTimesProject.batchedMultiplyAoS(
    fineGridValues, // image
    coarseGridValues,  // preimage
    unknowns,          // batch size, i.e. how often to apply it in one AoS rush
    numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, // result size, i.e. size of image
    0
  );
  #else
  PickTimesProject.batchedMultiplyAoS(
    fineGridValues, // image
    coarseGridValues,  // preimage
    unknowns,          // batch size, i.e. how often to apply it in one AoS rush
    numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch, // result size, i.e. size of image
    0
  );
  #endif

  dfor(cell,numberOfDoFsPerAxisInPatch) {
    int cellSerialised = peano4::utils::dLinearised(cell,numberOfDoFsPerAxisInPatch);
    for (int j=0; j<5; j++) {
      assertion6(
        fineGridValues[cellSerialised*unknowns+j]==fineGridValues[cellSerialised*unknowns+j],
        cellSerialised, j, unknowns, numberOfDoFsPerAxisInPatch, cell, marker.toString()
      );
    }
  }
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_linear(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridFaceValues,
  double*                                   coarseGridFaceValues,
  double*                                   coarseGridCellValues
) {
  logTraceInWith4Arguments( "interpolateHaloLayer_AoS_linear(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns );

  int normal = marker.getSelectedFaceNumber() % Dimensions;
  if (
    marker.getRelativePositionWithinFatherCell()(normal)==0
    or
    marker.getRelativePositionWithinFatherCell()(normal)==3
  ) {
    interpolateHaloLayer_AoS_linear(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      unknowns,
      fineGridFaceValues,
      coarseGridFaceValues
    );
  }
  else {
    tarch::la::DynamicMatrix Project = internal::createLinearInterpolationMatrix(numberOfDoFsPerAxisInPatch);

    #if Dimensions==2
    double* interpolatedValues = tarch::allocateMemory(3*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*unknowns,tarch::MemoryLocation::Heap);
    Project.batchedMultiplyAoS(
      interpolatedValues, // image
      coarseGridCellValues,  // preimage
      unknowns
    );
    #else
    double* interpolatedValues = tarch::allocateMemory(3*3*3*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*numberOfDoFsPerAxisInPatch*unknowns,tarch::MemoryLocation::Heap);
    Project.batchedMultiplyAoS(
      interpolatedValues, // image
      coarseGridCellValues,  // preimage
      unknowns
    );
    #endif

    internal::projectInterpolatedFineCellsOnHaloLayer_AoS(
      marker,
      numberOfDoFsPerAxisInPatch,
      overlap,
      [&](
        tarch::la::Vector<Dimensions,int> coarseVolume,
        tarch::la::Vector<Dimensions,int> fineVolume,
/*
        tarch::la::Vector<Dimensions,double> coarseVolumeCentre,
        tarch::la::Vector<Dimensions,double> fineVolumeCentre,
*/
        double coarseVolumeH,
        double fineVolumeH
      )->void {
        int coarseVolumeLinearised = peano4::utils::dLinearised(
          coarseVolume,
          numberOfDoFsPerAxisInPatch*3
        );
        int fineVolumeLinearised = serialisePatchIndexInOverlap(
          fineVolume,
          numberOfDoFsPerAxisInPatch, overlap, normal
        );
        for (int j=0; j<unknowns; j++) {
          fineGridFaceValues[fineVolumeLinearised*unknowns+j] = interpolatedValues[coarseVolumeLinearised*unknowns+j];
        }
        assertion10(
          fineGridFaceValues[fineVolumeLinearised*unknowns]==fineGridFaceValues[fineVolumeLinearised*unknowns],
		  fineGridFaceValues[fineVolumeLinearised*unknowns],
		  coarseVolume, fineVolume, coarseVolumeLinearised, fineVolumeLinearised,
		  marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns,
		  Project.toString(true)
		);
      }
    );

    tarch::freeMemory(interpolatedValues,tarch::MemoryLocation::Heap);
  }

  logTraceOut( "interpolateHaloLayer_AoS_linear(...)" );
}


void toolbox::blockstructured::interpolateHaloLayer_AoS_outflow(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  assertionEquals(overlap,1);

  interpolateHaloLayer_AoS_linear(
    marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues
  );

  const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;
  const bool pickLeftHalfOfHaloOnFineGrid  = (marker.getSelectedFaceNumber() < Dimensions);

  dfore(fineVolumeOutside,numberOfDoFsPerAxisInPatch,normal,pickLeftHalfOfHaloOnFineGrid ? 0 : 1) {
    tarch::la::Vector<Dimensions,int>    fineVolumeInside = fineVolumeOutside;
    fineVolumeInside(normal) = pickLeftHalfOfHaloOnFineGrid ? 1 : 0;

    int fineVolumeOutsideLinearised = serialisePatchIndexInOverlap(
      fineVolumeOutside,
      numberOfDoFsPerAxisInPatch, overlap, normal
      );
    int fineVolumeInsideLinearised = serialisePatchIndexInOverlap(
      fineVolumeInside,
      numberOfDoFsPerAxisInPatch, overlap, normal
      );

    for (int j=0; j<unknowns; j++) {
      fineGridValues[fineVolumeOutsideLinearised*unknowns+j] = fineGridValues[fineVolumeInsideLinearised*unknowns+j];
    }
  }
}


void toolbox::blockstructured::interpolateCell_AoS_outflow(
  const peano4::datamanagement::CellMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  toolbox::blockstructured::interpolateCell_AoS_piecewise_constant(
    marker,
    numberOfDoFsPerAxisInPatch,
    unknowns,
    fineGridValues,
    coarseGridValues
  );
}


void toolbox::blockstructured::restrictOntoOuterHalfOfHaloLayer_AoS_inject(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues,
  bool                                      swapInsideOutside
) {
  logTraceInWith6Arguments( "restrictOntoOuterHalfOfHaloLayer_AoS_inject(...)", marker.toString(), numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues );

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

  logTraceOut( "restrictOntoOuterHalfOfHaloLayer_AoS_restrict(...)" );
}


void toolbox::blockstructured::restrictHaloLayer_AoS_inject(
  const peano4::datamanagement::FaceMarker& marker,
  int                                       numberOfDoFsPerAxisInPatch,
  int                                       overlap,
  int                                       unknowns,
  double*                                   fineGridValues,
  double*                                   coarseGridValues
) {
  restrictOntoOuterHalfOfHaloLayer_AoS_inject( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, false );
  restrictOntoOuterHalfOfHaloLayer_AoS_inject( marker, numberOfDoFsPerAxisInPatch, overlap, unknowns, fineGridValues, coarseGridValues, true );
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
