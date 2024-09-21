#include "IandRSecondOrderTest.h"
#include "../Interpolation.h"
#include "../Restriction.h"

#include <fstream>
#include <iostream>
#include <random>
#include <cmath>

toolbox::blockstructured::tests::IandRSecondOrderTest::IandRSecondOrderTest():
  TestCase("toolbox::blockstructured::tests::IandRSecondOrderTest") {
}

void toolbox::blockstructured::tests::IandRSecondOrderTest::run() {
  testMethod(compareInterpolateLinearCaseToTensorProduct);
  testMethod(compareRestrictLinearCaseToTensorProduct);
}

void toolbox::blockstructured::tests::IandRSecondOrderTest::compareInterpolateLinearCaseToTensorProduct(){
  double normalInterp[] = {
    0.0000, 0.3333, 0.6667, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.6667, 0.3333, 0.0000, 0.0000
  };

  double tangentInterp[] = {
    1.3333, -0.3333, 0.0000, 0.0000, 0.0000,
    1.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.6667, 0.3333, 0.0000, 0.0000, 0.0000,
    0.3333, 0.6667, 0.0000, 0.0000, 0.0000,
    0.0000, 1.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.6667, 0.3333, 0.0000, 0.0000,
    0.0000, 0.3333, 0.6667, 0.0000, 0.0000,
    0.0000, 0.0000, 1.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.6667, 0.3333, 0.0000,
    0.0000, 0.0000, 0.3333, 0.6667, 0.0000,
    0.0000, 0.0000, 0.0000, 1.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.6667, 0.3333,
    0.0000, 0.0000, 0.0000, 0.3333, 0.6667,
    0.0000, 0.0000, 0.0000, 0.0000, 1.0000,
    0.0000, 0.0000, 0.0000, -0.3333, 1.3333
  };

  constexpr int patchSize = 5;
  constexpr int overlap = 3;
  constexpr int unknowns = 59;

  for (int i = 0; i < Dimensions; i++) {
    for (int j = 0; j < Dimensions; j++) {
#if Dimensions == 3
      for (int k = 0; k < 3; k++) {
#endif
        peano4::grid::GridTraversalEvent dummyEvent;
        dummyEvent.setRelativePositionToFather(tarch::la::Vector<Dimensions, int>{i, j,
#if Dimensions == 3
        k
#endif
        });

        for (int axis = 0; axis < Dimensions; axis++) {
          peano4::datamanagement::FaceMarker faceMarker1(dummyEvent, axis);
          peano4::datamanagement::FaceMarker faceMarker2(dummyEvent, axis + 3);

          double* coarseData = new double[unknowns * patchSize * patchSize * overlap * 2];
          double* fineDataTensorProduct = new double[unknowns * patchSize * patchSize * overlap * 2];
          double* fineDataSecondOrder = new double[unknowns * patchSize * patchSize * overlap * 2];
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) coarseData[n] = (double)(n + 1) / unknowns;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) fineDataTensorProduct[n] = 0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) fineDataSecondOrder[n] = 0;

          interpolateHaloLayer_AoS_tensor_product(faceMarker1, patchSize, overlap, unknowns, normalInterp, tangentInterp, coarseData, fineDataTensorProduct);
          interpolateHaloLayer_AoS_second_order(faceMarker1, patchSize, overlap, unknowns, coarseData, fineDataSecondOrder);
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) validateNumericalEqualsWithEpsWithParams3(fineDataTensorProduct[n], fineDataSecondOrder[n], 0.01, faceMarker1.getRelativePositionWithinFatherFace(), axis, n);

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) fineDataTensorProduct[n] = 0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) fineDataSecondOrder[n] = 0;

          interpolateHaloLayer_AoS_tensor_product(faceMarker2, patchSize, overlap, unknowns, normalInterp, tangentInterp, coarseData, fineDataTensorProduct);
          interpolateHaloLayer_AoS_second_order(faceMarker2, patchSize, overlap, unknowns, coarseData, fineDataSecondOrder);
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) validateNumericalEqualsWithEpsWithParams3(fineDataTensorProduct[n], fineDataSecondOrder[n], 0.01, faceMarker1.getRelativePositionWithinFatherFace(), axis + 3, n);

          delete[] coarseData;
          delete[] fineDataTensorProduct;
          delete[] fineDataSecondOrder;
        }
#if Dimensions == 3
      }
#endif
    }
  }
}

void toolbox::blockstructured::tests::IandRSecondOrderTest::compareRestrictLinearCaseToTensorProduct(){
double normalRestrict[] = {0.0000,  0.0000,  0.0000,  0.333333,  0.333333,  0.333333,
0.0000,  0.0000,  0.0000,  0.0000,  -2.0000,  3.0000,
0.0000,  0.0000,  0.0000,  0.0000,  -5.0000,  6.0000};

double tangentRestrict[] = {0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000,
0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000};


int patchSize = 5;
int overlap = 3;
int unknowns = 59;

for (int i = 0; i < 3; i++){
  for (int j = 0; j < 3; j++){
    for (int k = 0; k < 3; k++){
      peano4::grid::GridTraversalEvent dummyEvent;
      dummyEvent.setRelativePositionToFather(tarch::la::Vector<Dimensions, int>{i, j, k});

      for (int axis = 0; axis < 3; axis++){
        peano4::datamanagement::FaceMarker faceMarker1(dummyEvent, axis);
        peano4::datamanagement::FaceMarker faceMarker2(dummyEvent, axis + 3);

	double* fineData = new double[unknowns * patchSize * patchSize * overlap * 2];
	double* coarseDataTensorProduct = new double[unknowns * patchSize * patchSize * overlap * 2];
	double* coarseDataSecondOrder = new double[unknowns * patchSize * patchSize * overlap * 2];
	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) fineData[n] = (double)(n + 1) / unknowns;
	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) coarseDataTensorProduct[n] = 0;
	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) coarseDataSecondOrder[n] = 0;

	restrictHaloLayer_AoS_tensor_product(faceMarker1, patchSize, overlap, unknowns, normalRestrict, tangentRestrict, fineData, coarseDataTensorProduct);
	restrictHaloLayer_AoS_second_order(faceMarker1, patchSize, overlap, unknowns, fineData, coarseDataSecondOrder);
	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) validateNumericalEqualsWithEpsWithParams3(coarseDataTensorProduct[n], coarseDataSecondOrder[n], 0.01, faceMarker1.getRelativePositionWithinFatherFace(), axis, n);

	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) coarseDataTensorProduct[n] = 0;
	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) coarseDataSecondOrder[n] = 0;

	restrictHaloLayer_AoS_tensor_product(faceMarker2, patchSize, overlap, unknowns, normalRestrict, tangentRestrict, fineData, coarseDataTensorProduct);
	restrictHaloLayer_AoS_second_order(faceMarker2, patchSize, overlap, unknowns, fineData, coarseDataSecondOrder);
	for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2; n++) validateNumericalEqualsWithEpsWithParams3(coarseDataTensorProduct[n], coarseDataSecondOrder[n], 0.01, faceMarker1.getRelativePositionWithinFatherFace(), axis + 3, n);
      }
    }
  }
}

}
