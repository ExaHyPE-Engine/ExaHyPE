// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InterpolationRestrictionTest.h"

#include "peano4/datamanagement/CellMarker.h"
#include "toolbox/blockstructured/Interpolation.h"
#include "toolbox/blockstructured/Restriction.h"

#include <algorithm>
#include <iomanip>

// @todo It seems as if these tests should be in the toolbox

exahype2::fv::tests::InterpolationRestrictionTest::InterpolationRestrictionTest():
  TestCase ("exahype2::fv::tests::InterpolationRestrictionTest") {
}


void exahype2::fv::tests::InterpolationRestrictionTest::run() {
  testMethod (testPiecewiseConstantInterpolationWithTensorProduct1);

  testMethod( testAverageRestrictionWithTensorProduct );
  testMethod( testInjectionExtrapolationRestrictionWithTensorProduct );

  // @todo Han copy in if yuo wanna test more things
  // testMethod (testPiecewiseConstantInterpolationWithTensorProduct2);
}


void exahype2::fv::tests::InterpolationRestrictionTest::testPiecewiseConstantInterpolationWithTensorProduct1() {
  static constexpr double  NormalInterpolationMatrix1d[]     = {
    0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0, 0.0, 0.0
  };
  static constexpr double  TangentialInterpolationMatrix1d[] = {
    1.0,0.0,0.0,0.0,0.0,
    1.0,0.0,0.0,0.0,0.0,
    1.0,0.0,0.0,0.0,0.0,
    0.0,1.0,0.0,0.0,0.0,
    0.0,1.0,0.0,0.0,0.0,

    0.0,1.0,0.0,0.0,0.0,
    0.0,0.0,1.0,0.0,0.0,
    0.0,0.0,1.0,0.0,0.0,
    0.0,0.0,1.0,0.0,0.0,
    0.0,0.0,0.0,1.0,0.0,

    0.0,0.0,0.0,1.0,0.0,
    0.0,0.0,0.0,1.0,0.0,
    0.0,0.0,0.0,0.0,1.0,
    0.0,0.0,0.0,0.0,1.0,
    0.0,0.0,0.0,0.0,1.0
  };


  #if Dimensions==2
  peano4::grid::GridTraversalEvent    dummEvent;
  dummEvent.setRelativePositionToFather( {0,0} );

  peano4::datamanagement::FaceMarker  marker(dummEvent);

  {
    double  coarseGridFaceValues[] = {
      0.1,-0.1, 0.2,-0.2, 0.3,-0.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      1.1,-1.1, 1.2,-1.2, 1.3,-1.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      2.1,-2.1, 2.2,-2.2, 2.3,-2.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      3.1,-3.1, 3.2,-3.2, 3.3,-3.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      4.1,-4.1, 4.2,-4.2, 4.3,-4.3,    0.0,0.0, 0.0,0.0, 0.0,0.0
    };
    double  fineGridFaceValues[ 3*5*2*2 ]; // three width, five dofs per axis, two unknowns, two sides
    std::fill_n(fineGridFaceValues, 3*5*2*2, 0.0 );

    marker.select(0);

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
      marker,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalInterpolationMatrix1d,
      TangentialInterpolationMatrix1d,
      coarseGridFaceValues,
      fineGridFaceValues
    );

    validateNumericalEquals( fineGridFaceValues[0*2+0],  0.2 );
    validateNumericalEquals( fineGridFaceValues[0*2+1], -0.2 );
    validateNumericalEquals( fineGridFaceValues[1*2+0],  0.2 );
    validateNumericalEquals( fineGridFaceValues[1*2+1], -0.2 );
    validateNumericalEquals( fineGridFaceValues[2*2+0],  0.2 );
    validateNumericalEquals( fineGridFaceValues[2*2+1], -0.2 );
    validateNumericalEquals( fineGridFaceValues[3*2+0],  0.0 );
    validateNumericalEquals( fineGridFaceValues[3*2+1],  0.0 );

    validateNumericalEquals( fineGridFaceValues[6*2+0],  0.2 );
    validateNumericalEquals( fineGridFaceValues[6*2+1], -0.2 );

    validateNumericalEquals( fineGridFaceValues[12*2+0],  0.2 );
    validateNumericalEquals( fineGridFaceValues[12*2+1], -0.2 );

    validateNumericalEquals( fineGridFaceValues[18*2+0],  1.2 );
    validateNumericalEquals( fineGridFaceValues[18*2+1], -1.2 );
  }

  {
    double  coarseGridFaceValues[] = {
      0.0,0.0,   0.0,0.0,   0.0,0.0,   0.0,0.0,   0.0,0.0,
      0.0,0.0,   0.0,0.0,   0.0,0.0,   0.0,0.0,   0.0,0.0,
      0.0,0.0,   0.0,0.0,   0.0,0.0,   0.0,0.0,   0.0,0.0,

      0.1,-0.1,  0.2,-0.2,  0.3,-0.3,  0.4,-0.4,  0.5,-0.5,
      1.1,-1.1,  1.2,-1.2,  1.3,-0.3,  1.4,-1.4,  1.5,-1.5,
      2.1,-2.1,  2.2,-2.2,  2.3,-0.3,  2.4,-2.4,  2.5,-2.5
    };
    double  fineGridFaceValues[ 3*5*2*2 ]; // three width, five dofs per axis, two unknowns, two sides
    std::fill_n(fineGridFaceValues, 3*5*2*2, 0.0 );

    marker.select(3);

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
      marker,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalInterpolationMatrix1d,
      TangentialInterpolationMatrix1d,
      coarseGridFaceValues,
      fineGridFaceValues
    );

    validateNumericalEquals( fineGridFaceValues[0*2+0],  0.0 );
    validateNumericalEquals( fineGridFaceValues[0*2+1],  0.0 );

    validateNumericalEquals( fineGridFaceValues[4*2+0],  0.0 );
    validateNumericalEquals( fineGridFaceValues[4*2+1],  0.0 );

    validateNumericalEquals( fineGridFaceValues[5*2+0],  0.0 );
    validateNumericalEquals( fineGridFaceValues[5*2+1],  0.0 );

    validateNumericalEquals( fineGridFaceValues[10*2+0],  0.0 );
    validateNumericalEquals( fineGridFaceValues[10*2+1],  0.0 );

    validateNumericalEquals( fineGridFaceValues[15*2+0],  1.1 );
    validateNumericalEquals( fineGridFaceValues[15*2+1], -1.1 );
  }
  #endif
}


void exahype2::fv::tests::InterpolationRestrictionTest::testAverageRestrictionWithTensorProduct(){
  // Testing a patch with size of 5. See header documentation: these are
  // the operators for the right face.
  static constexpr double  NormalRestrictionMatrix1d[]     = {
    0.0,    0.0,    0.0,    1.0/3.0,    1.0/3.0,    1.0/3.0,
    0.0,    0.0,    0.0,    1.0/3.0,    1.0/3.0,    1.0/3.0,
    0.0,    0.0,    0.0,    1.0/3.0,    1.0/3.0,    1.0/3.0
  };
/*
  static constexpr double  NormalRestrictionMatrix1d[]     = {
    6.0,     -5.0,    0.0,     0.0, 0.0, 0.0,
    3.0,     -2.0,    0.0,     0.0, 0.0, 0.0,
    1.0/3.0, 1.0/3.0, 1.0/3.0, 0.0, 0.0, 0.0
  };
*/
  static constexpr double  TangentialRestrictionMatrix1d[] = {
    1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0 
  };

  peano4::grid::GridTraversalEvent    dummEvent;
  int face=3;
  #if Dimensions==2
  dummEvent.setRelativePositionToFather( {0,0} );

  //peano4::datamanagement::FaceMarker  marker(dummEvent);
  //marker.select(face);

  constexpr int NumberOfFaceValues = 3*5 * 2 * 2; // three width, five dofs per axis, two unknowns, two sides

  double  fineGridFaceValues[] = {
    0.1,-0.1, 0.1,-0.1, 0.1,-0.1,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    0.1,-0.1, 0.1,-0.1, 0.1,-0.1,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    0.1,-0.1, 0.1,-0.1, 0.1,-0.1,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    0.1,-0.1, 0.1,-0.1, 0.1,-0.1,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    0.1,-0.1, 0.1,-0.1, 0.1,-0.1,    0.0,0.0, 0.0,0.0, 0.0,0.0
  };
  #else
  dummEvent.setRelativePositionToFather( {0,0,0} );

  constexpr int NumberOfFaceValues = 3*5*5 * 2 * 2; // three width, five dofs per axis, two unknowns, two sides

  double  fineGridFaceValues[] = {
    10.1,-10.1, 10.2,-10.2, 10.3,-10.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    11.1,-11.1, 11.2,-11.2, 11.3,-11.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    12.1,-12.1, 12.2,-12.2, 12.3,-12.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    13.1,-13.1, 13.2,-13.2, 13.3,-13.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    14.1,-14.1, 14.2,-14.2, 14.3,-14.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,

    20.1,-20.1, 20.2,-20.2, 20.3,-20.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    21.1,-21.1, 21.2,-21.2, 21.3,-21.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    22.1,-22.1, 22.2,-22.2, 22.3,-22.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    23.1,-23.1, 23.2,-23.2, 23.3,-23.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    24.1,-24.1, 24.2,-24.2, 24.3,-24.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,

    30.1,-30.1, 30.2,-30.2, 30.3,-30.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    31.1,-31.1, 31.2,-31.2, 31.3,-31.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    32.1,-32.1, 32.2,-32.2, 32.3,-32.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    33.1,-33.1, 33.2,-33.2, 33.3,-33.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    34.1,-34.1, 34.2,-34.2, 34.3,-34.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,

    40.1,-40.1, 40.2,-40.2, 40.3,-40.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    41.1,-41.1, 41.2,-41.2, 41.3,-41.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    42.1,-42.1, 42.2,-42.2, 42.3,-42.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    43.1,-43.1, 43.2,-43.2, 43.3,-43.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    44.1,-44.1, 44.2,-44.2, 44.3,-44.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,

    50.1,-50.1, 50.2,-50.2, 50.3,-50.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    51.1,-51.1, 51.2,-51.2, 51.3,-51.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    52.1,-52.1, 52.2,-52.2, 52.3,-52.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    53.1,-53.1, 53.2,-53.2, 53.3,-53.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
    54.1,-54.1, 54.2,-54.2, 54.3,-54.3,    0.0,0.0, 0.0,0.0, 0.0,0.0
  };
  #endif

  double  coarseGridFaceValues[ NumberOfFaceValues ];
  peano4::datamanagement::FaceMarker  marker1(dummEvent);
  marker1.select(Dimensions);

  std::fill_n(coarseGridFaceValues, NumberOfFaceValues, 0.0 );

  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker1,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  #if Dimensions==2
  validateNumericalEqualsWithParams6(
    coarseGridFaceValues[0*2+0],  0.1,
    coarseGridFaceValues[0*2+0], coarseGridFaceValues[1*2+0], coarseGridFaceValues[2*2+0],
    coarseGridFaceValues[3*2+0], coarseGridFaceValues[4*2+0], coarseGridFaceValues[5*2+0]
  );
  #endif
}


void exahype2::fv::tests::InterpolationRestrictionTest::testPiecewiseConstantInterpolationWithTensorProduct2() {

  //@tobias: you can just ignore it as I am just testing the schme in my stupid way.

  static constexpr double  NormalInterpolationMatrix1d[]     = {
    0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0, 0.0
  };
  static constexpr double  TangentialInterpolationMatrix1d[] = {
    1.0,0.0,0.0,0.0,0.0,
    1.0,0.0,0.0,0.0,0.0,
    1.0,0.0,0.0,0.0,0.0,
    0.0,1.0,0.0,0.0,0.0,
    0.0,1.0,0.0,0.0,0.0,

    0.0,1.0,0.0,0.0,0.0,
    0.0,0.0,1.0,0.0,0.0,
    0.0,0.0,1.0,0.0,0.0,
    0.0,0.0,1.0,0.0,0.0,
    0.0,0.0,0.0,1.0,0.0,

    0.0,0.0,0.0,1.0,0.0,
    0.0,0.0,0.0,1.0,0.0,
    0.0,0.0,0.0,0.0,1.0,
    0.0,0.0,0.0,0.0,1.0,
    0.0,0.0,0.0,0.0,1.0
  };


  #if Dimensions==2
  peano4::grid::GridTraversalEvent    dummEvent;
  dummEvent.setRelativePositionToFather( {0,2} );

  peano4::datamanagement::FaceMarker  marker(dummEvent);

  {
    double  coarseGridFaceValues[] = {
      0.1,-0.1, 0.2,-0.2, 0.3,-0.3,    5.1,-5.1, 5.2,-5.2, 5.3,-5.3,
      1.1,-1.1, 1.2,-1.2, 1.3,-1.3,    6.1,-6.1, 6.2,-6.2, 6.3,-6.3,
      2.1,-2.1, 2.2,-2.2, 2.3,-2.3,    7.1,-7.1, 7.2,-7.2, 7.3,-7.3,
      3.1,-3.1, 3.2,-3.2, 3.3,-3.3,    8.1,-8.1, 8.2,-8.2, 8.3,-8.3, 
      4.1,-4.1, 4.2,-4.2, 4.3,-4.3,    9.1,-9.1, 9.2,-9.2, 9.3,-9.3
    };
    double  fineGridFaceValues[ 3*5*2*2 ]; // three width, five dofs per axis, two unknowns, two sides
    std::fill_n(fineGridFaceValues, 3*5*2*2, 0.0 );

    marker.select(0);

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
      marker,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalInterpolationMatrix1d,
      TangentialInterpolationMatrix1d,
      coarseGridFaceValues,
      fineGridFaceValues
    );

    std::cout<<std::setprecision(1);
    for (int i=0;i<5;i++){
      for (int j=0;j<12;j++){
        std::cout<<fineGridFaceValues[i*12+j] << " ";
      }
      std::cout<<"\n";
    }
  }
/*
  {
    double  coarseGridFaceValues[] = {
      0.1,-0.1, 0.2,-0.2, 0.3,-0.3, 0.4,-0.4, 0.5,-0.5,
      1.1,-1.1, 1.2,-1.2, 1.3,-1.3, 1.4,-1.4, 1.5,-1.5,
      2.1,-2.1, 2.2,-2.2, 2.3,-2.3, 2.4,-2.4, 2.5,-2.5,

      3.1,-3.1, 3.2,-3.2, 3.3,-3.3, 3.4,-3.4, 3.5,-3.5,
      4.1,-4.1, 4.2,-4.2, 4.3,-4.3, 4.4,-4.4, 4.5,-4.5,
      5.1,-5.1, 5.2,-5.2, 5.3,-5.3, 5.4,-5.4, 5.5,-5.5
    };
    double  fineGridFaceValues[ 3*5*2*2 ]; // three width, five dofs per axis, two unknowns, two sides
    std::fill_n(fineGridFaceValues, 3*5*2*2, 0.0 );

    marker.select(1);

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_tensor_product(
      marker,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalInterpolationMatrix1d,
      TangentialInterpolationMatrix1d,
      coarseGridFaceValues,
      fineGridFaceValues
    );

    std::cout<<std::setprecision(1);
    for (int i=0;i<6;i++){
      for (int j=0;j<10;j++){
        std::cout<<fineGridFaceValues[i*10+j] << " ";
      }
      std::cout<<"\n";
    }
  }*/
  #endif
}


void exahype2::fv::tests::InterpolationRestrictionTest::testInjectionExtrapolationRestrictionWithTensorProduct(){
  static constexpr double TangentialRestrictionMatrix1d[] = {
    0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 ,0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};

  static constexpr double NormalRestrictionMatrix1d[] = {
    0.0, 0.0, 0.0, 0.3333333333333333, 0.3333333333333333, 0.3333333333333333,
    0.0, 0.0, 0.0, 0.0, -2.0, 3.0,
    0.0, 0.0, 0.0, 0.0, -5.0, 6.0};

  peano4::grid::GridTraversalEvent    dummEvent;
  #if Dimensions==2
  dummEvent.setRelativePositionToFather( {0,0} );

  constexpr int NumberOfFaceValues = 3*5 * 2 * 2; // three width, five dofs per axis, two unknowns, two sides

  double  fineGridFaceValues[] = {
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,

    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
  };
  #else
  dummEvent.setRelativePositionToFather( {0,0,2} );

  constexpr int NumberOfFaceValues = 3*5*5 * 5 * 2; // three width, five dofs per axis, five unknowns, two sides

  double  fineGridFaceValues[] = {
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,

    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,

    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,
    0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0, 0.1,0,0,0,0,

    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,

    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,

    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0
  };
  #endif

  double  coarseGridFaceValues[ NumberOfFaceValues ];
  peano4::datamanagement::FaceMarker  marker1(dummEvent);
  marker1.select(2*Dimensions-1);

  std::fill_n(coarseGridFaceValues, NumberOfFaceValues, 0.0 );

  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker1,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  #if Dimensions==2
  validateNumericalEqualsWithParams6(
    coarseGridFaceValues[0*2+0],  0.1,
    coarseGridFaceValues[0*2+0], coarseGridFaceValues[1*2+0], coarseGridFaceValues[2*2+0],
    coarseGridFaceValues[3*2+0], coarseGridFaceValues[4*2+0], coarseGridFaceValues[5*2+0]
  );
  #else
  validateNumericalEquals( coarseGridFaceValues[0*5+0],  0.1 );
  validateNumericalEquals( coarseGridFaceValues[1*5+0],  0.1 );
  validateNumericalEquals( coarseGridFaceValues[5*5+0],  0.1 );
  validateNumericalEquals( coarseGridFaceValues[6*5+0],  0.1 );

  dummEvent.setRelativePositionToFather( {1,0,2} );
  peano4::datamanagement::FaceMarker  marker102(dummEvent);
  marker102.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker102,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {2,0,2} );
  peano4::datamanagement::FaceMarker  marker202(dummEvent);
  marker202.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker202,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {0,1,2} );
  peano4::datamanagement::FaceMarker  marker012(dummEvent);
  marker012.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker012,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {1,1,2} );
  peano4::datamanagement::FaceMarker  marker112(dummEvent);
  marker112.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker112,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {2,1,2} );
  peano4::datamanagement::FaceMarker  marker212(dummEvent);
  marker212.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker212,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {0,2,2} );
  peano4::datamanagement::FaceMarker  marker022(dummEvent);
  marker022.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker022,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {1,2,2} );
  peano4::datamanagement::FaceMarker  marker122(dummEvent);
  marker122.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker122,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  dummEvent.setRelativePositionToFather( {2,2,2} );
  peano4::datamanagement::FaceMarker  marker222(dummEvent);
  marker222.select(2*Dimensions-1);
  ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      marker222,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      5,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
  );

  for (int i=0; i<5*5*3; i++) {
    validateNumericalEquals( coarseGridFaceValues[i*5+0],  0.1 );
  }
  #endif
}


