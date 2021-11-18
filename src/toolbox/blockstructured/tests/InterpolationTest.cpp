#include "InterpolationTest.h"
#include "toolbox/blockstructured/Interpolation.h"


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


toolbox::blockstructured::tests::InterpolationTest::InterpolationTest():
  TestCase("toolbox::blockstructured::tests::InterpolationTest") {
}


void toolbox::blockstructured::tests::InterpolationTest::run() {
  testMethod(testRestrictHaloLayer_AoS_averaging);
  testMethod(testRestrictCellForBreakingDam);
}


void toolbox::blockstructured::tests::InterpolationTest::testRestrictCellForBreakingDam() {
  #if Dimensions==2 and PeanoDebug>0
  peano4::grid::GridTraversalEvent    dummyEvent;
  peano4::datamanagement::CellMarker  cellMarker(dummyEvent);
  cellMarker.setRelativePositionWithinFatherCell( 0,0 );
  cellMarker.setRelativePositionWithinFatherCell( 1,0 );
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
  #if Dimensions==2
  peano4::grid::GridTraversalEvent    dummyEvent;
  peano4::datamanagement::FaceMarker  faceMarker(dummyEvent);
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

#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
