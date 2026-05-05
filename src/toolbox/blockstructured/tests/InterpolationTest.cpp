#include "InterpolationTest.h"

#include "../Interpolation.h"
#include "../Restriction.h"

#include "peano4/grid/GridTraversalEvent.h"

toolbox::blockstructured::tests::InterpolationTest::InterpolationTest():
  TestCase("toolbox::blockstructured::tests::InterpolationTest") {
}


void toolbox::blockstructured::tests::InterpolationTest::run() {
  testMethod(testInterpolateHaloLayer_AoS_piecewise_constant_withSingleVolumePatch);
  testMethod(testRestrictHaloLayer_AoS_averaging);
  testMethod(testRestrictCellForBreakingDam);
  testMethod(testInterpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear);
}


void toolbox::blockstructured::tests::InterpolationTest::testInterpolateHaloLayer_AoS_piecewise_constant_withSingleVolumePatch() {
  peano4::grid::GridTraversalEvent event;
#if DIMENSIONS==2
  event.setRelativePositionToFather( {0,0} );
#else
  event.setRelativePositionToFather( {0,0,0} );
#endif
  peano4::datamanagement::FaceMarker faceMarker = event.getFaceMarker();
  faceMarker.select(0);

  double coarseGridValues[] = {3.0, 7.0};
  double fineGridValues[]   = {0.0, 0.0};

  toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
    faceMarker,
    1,
    1,
    1,
    coarseGridValues,
    fineGridValues
  );

  validateNumericalEquals( fineGridValues[0], 3.0 );
  validateNumericalEquals( fineGridValues[1], 7.0 );
}


void toolbox::blockstructured::tests::InterpolationTest::testInterpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear() {
  #if DIMENSIONS==3
  const int NumberOfUnknowns = 3;
  const int SourcePatchSize = 4;
  const int SourceHaloSize = 3;
  const int DestinationPatchSize = 16;
  const int DestinationHaloSize = 1;

  int SourceSize = (SourcePatchSize+2*SourceHaloSize) * (SourcePatchSize+2*SourceHaloSize) * (SourcePatchSize+2*SourceHaloSize) * NumberOfUnknowns;
  double* sourceData = new double[ SourceSize ];
  for (int i=0; i<SourceSize; i++) sourceData[i] = 1.0;

  int DestinationSize = (DestinationPatchSize+2*DestinationHaloSize) * (DestinationPatchSize+2*DestinationHaloSize) * (DestinationPatchSize+2*DestinationHaloSize) * NumberOfUnknowns;
  double* destinationData = new double[ DestinationSize ];

  interpolateCellDataAssociatedToVolumesIntoOverlappingCell_linear(
    SourcePatchSize,
    DestinationPatchSize,
    SourceHaloSize,
    DestinationHaloSize,
    NumberOfUnknowns,
    sourceData,
    destinationData
  );

  for (int i=0; i<DestinationSize; i++) {
    validateNumericalEqualsWithParams1( destinationData[i], 1.0, i );
  }

  delete[] sourceData;
  delete[] destinationData;
  #endif
}


void toolbox::blockstructured::tests::InterpolationTest::testRestrictCellForBreakingDam() {
#if DIMENSIONS==2
  peano4::datamanagement::CellMarker cellMarker;

  double fineGridValues[] =
      {
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.0, 0.0, 0.0, 0.0, 0.0,

        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.0, 0.0, 0.0, 0.0, 0.0,

        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.0, 0.0, 0.0, 0.0, 0.0,

        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.1, 0.2, 0.3, 0.4, 0.5,
        0.0, 0.0, 0.0, 0.0, 0.0
      };

  double coarseGridValues[4*4*5];

  std::fill_n(coarseGridValues,4*4*5,0.0);

  ::toolbox::blockstructured::restrictCell_AoS_averaging(
      cellMarker,
      4,
      5,
      fineGridValues,
      coarseGridValues
  );

  validateNumericalEquals( coarseGridValues[0*5+0], 0.1 );
  validateNumericalEquals( coarseGridValues[0*5+1], 0.2 );
  validateNumericalEquals( coarseGridValues[0*5+2], 0.3 );
  validateNumericalEquals( coarseGridValues[0*5+3], 0.4 );
  validateNumericalEquals( coarseGridValues[0*5+4], 0.5 );

  validateNumericalEquals( coarseGridValues[1*5+0], 0.0 );
  validateNumericalEquals( coarseGridValues[1*5+1], 0.0 );
  validateNumericalEquals( coarseGridValues[1*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[1*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[1*5+4], 0.0 );

  validateNumericalEquals( coarseGridValues[2*5+0], 0.0 );
  validateNumericalEquals( coarseGridValues[2*5+1], 0.0 );
  validateNumericalEquals( coarseGridValues[2*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[2*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[2*5+4], 0.0 );

  validateNumericalEquals( coarseGridValues[4*5+0], 0.1/3.0 );
  validateNumericalEquals( coarseGridValues[4*5+1], 0.2/3.0 );
  validateNumericalEquals( coarseGridValues[4*5+2], 0.3/3.0 );
  validateNumericalEquals( coarseGridValues[4*5+3], 0.4/3.0 );
  validateNumericalEquals( coarseGridValues[4*5+4], 0.5/3.0 );
  #endif
}


void toolbox::blockstructured::tests::InterpolationTest::testRestrictHaloLayer_AoS_averaging() {
  #if DIMENSIONS==2
  peano4::grid::GridTraversalEvent    event;
  event.setRelativePositionToFather(0,0);
  event.setRelativePositionToFather(1,0);
  peano4::datamanagement::FaceMarker  faceMarker = event.getFaceMarker();

  double fineGridValues[] =
      {
       0.100047, 0.000933957, 0,0,0.00493399,
       0.100045, 0.000870917, 0,0,0.0045799,
       0.100039, 0.000801622, 0,0,0.00422816,
       0.100045, 0.00081321,  0,0,0.0042177,
       0.100047, 0.000933957, 0,0,0.00493399,
       0.100045, 0.000870917, 2.71051e-20,0,0.0045799,
       0.100039, 0.000801622, 0,0,0.00422816,
       0.100045,0.00081321,0,0,0.0042177
      };
  double coarseGridValues[] =
      {
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0
      };

  toolbox::blockstructured::restrictHaloLayer_AoS_averaging(
    faceMarker,
    4, // int                                       numberOfDoFsPerAxisInPatch,
    1, // int                                       overlap,
    5, // int                                       unknowns,
    fineGridValues, coarseGridValues
  );

  validateNumericalEquals( coarseGridValues[0*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[0*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[1*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[1*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[2*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[2*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[3*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[3*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[4*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[4*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[5*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[5*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[6*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[6*5+3], 0.0 );
  validateNumericalEquals( coarseGridValues[7*5+2], 0.0 );
  validateNumericalEquals( coarseGridValues[7*5+3], 0.0 );

  #endif
}
