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
  testMethod (testPiecewiseConstantInterpolation);
  testMethod (testAverageRestriction);
}


void exahype2::fv::tests::InterpolationRestrictionTest::testPiecewiseConstantInterpolation() {
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

    validateNumericalEquals( fineGridFaceValues[0*2+0],  0.3 );
    validateNumericalEquals( fineGridFaceValues[0*2+1], -0.3 );
    validateNumericalEquals( fineGridFaceValues[1*2+0],  0.3 );
    validateNumericalEquals( fineGridFaceValues[1*2+1], -0.3 );
    validateNumericalEquals( fineGridFaceValues[2*2+0],  0.3 );
    validateNumericalEquals( fineGridFaceValues[2*2+1], -0.3 );
    validateNumericalEquals( fineGridFaceValues[3*2+0],  0.0 );
    validateNumericalEquals( fineGridFaceValues[3*2+1],  0.0 );

    validateNumericalEquals( fineGridFaceValues[6*2+0],  0.3 );
    validateNumericalEquals( fineGridFaceValues[6*2+1], -0.3 );

    validateNumericalEquals( fineGridFaceValues[12*2+0],  0.3 );
    validateNumericalEquals( fineGridFaceValues[12*2+1], -0.3 );

    validateNumericalEquals( fineGridFaceValues[18*2+0],  1.3 );
    validateNumericalEquals( fineGridFaceValues[18*2+1], -1.3 );
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

    validateNumericalEquals( fineGridFaceValues[15*2+0],  0.1 );
    validateNumericalEquals( fineGridFaceValues[15*2+1], -0.1 );
  }
  #endif
}

void exahype2::fv::tests::InterpolationRestrictionTest::testAverageRestriction(){
  //testing a patch with size of 5
  static constexpr double  NormalRestrictionMatrix1d[]     = {
    6.0,     -5.0,    0.0,     0.0, 0.0, 0.0,
    3.0,     -2.0,    0.0,     0.0, 0.0, 0.0,
    1.0/3.0, 1.0/3.0, 1.0/3.0, 0.0, 0.0, 0.0
  };
  static constexpr double  TangentialRestrictionMatrix1d[] = { //5 \times 15 matrix
    1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0/3.0, 1.0/3.0, 1.0/3.0 
  };

  {
    double  fineGridFaceValues[] = {
      0.1,-0.1, 0.2,-0.2, 0.3,-0.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      1.1,-1.1, 1.2,-1.2, 1.3,-1.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      2.1,-2.1, 2.2,-2.2, 2.3,-2.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      3.1,-3.1, 3.2,-3.2, 3.3,-3.3,    0.0,0.0, 0.0,0.0, 0.0,0.0,
      4.1,-4.1, 4.2,-4.2, 4.3,-4.3,    0.0,0.0, 0.0,0.0, 0.0,0.0
    };
    double  coarseGridFaceValues[ 3*5*2*2 ]; // three width, five dofs per axis, two unknowns, two sides
    std::fill_n(coarseGridFaceValues, 3*5*2*2, 0.0 );

    peano4::grid::GridTraversalEvent    dummEvent;
    dummEvent.setRelativePositionToFather( {0,0} );
    peano4::datamanagement::FaceMarker  marker1(dummEvent);
    marker1.select(0);

    ::toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product(
      marker1,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
    );

    dummEvent.setRelativePositionToFather( {0,1} );
    peano4::datamanagement::FaceMarker  marker2(dummEvent);
    marker2.select(0);
    
    ::toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product(
      marker2,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
    );

    dummEvent.setRelativePositionToFather( {0,2} );
    peano4::datamanagement::FaceMarker  marker3(dummEvent);
    marker3.select(0);
    
    ::toolbox::blockstructured::restrictHaloLayer_AoS_tensor_product(
      marker3,
      5,         // numberOfDoFsPerAxisInPatch
      3,         // overlap
      2,         // unknowns
      NormalRestrictionMatrix1d,
      TangentialRestrictionMatrix1d,
      fineGridFaceValues,
      coarseGridFaceValues
    );
/*
    std::cout<<std::setprecision(1);
    for (int i=0;i<5;i++){
      for (int j=0;j<12;j++){
        std::cout<<coarseGridFaceValues[i*12+j] << " ";
      }
      std::cout<<"\n";
    }

  }
}