// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "IandRSecondOrderTest.h"

#include "../Interpolation.h"
#include "../Restriction.h"

toolbox::blockstructured::tests::IandRSecondOrderTest::IandRSecondOrderTest():
  TestCase("toolbox::blockstructured::tests::IandRSecondOrderTest") {}

void toolbox::blockstructured::tests::IandRSecondOrderTest::run() {
  testMethod(compareInterpolateLinearCaseToTensorProduct);
  testMethod(compareRestrictLinearCaseToTensorProduct);
}

void toolbox::blockstructured::tests::IandRSecondOrderTest::compareInterpolateLinearCaseToTensorProduct(){
  const double normalInterp[] = {
    0.0000, 0.3333, 0.6667, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.6667, 0.3333, 0.0000, 0.0000
  };

  const double tangentInterp[] = {
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
  constexpr int overlap   = 3;
  constexpr int unknowns  = 59;
  const int     faceSize  = unknowns * patchSize * patchSize * overlap * 2;

  double coarseData[faceSize];
  double fineDataTensorProduct[faceSize];
  double fineDataSecondOrder[faceSize];

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        peano4::grid::GridTraversalEvent dummyEvent;
        dummyEvent.setRelativePositionToFather(
          tarch::la::Vector<Dimensions, int>{i, j, k}
        );

        for (int axis = 0; axis < 3; axis++) {
          peano4::datamanagement::FaceMarker faceMarker1(dummyEvent, axis);
          peano4::datamanagement::FaceMarker
            faceMarker2(dummyEvent, axis + Dimensions);

          for (int n = 0; n < faceSize; n++)
            coarseData[n] = static_cast<double>(n + 1) / 16.0;
          for (int n = 0; n < faceSize; n++)
            fineDataTensorProduct[n] = 0;
          for (int n = 0; n < faceSize; n++)
            fineDataSecondOrder[n] = 0;

          interpolateHaloLayer_AoS_tensor_product(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            normalInterp,
            tangentInterp,
            coarseData,
            fineDataTensorProduct
          );
          interpolateHaloLayer_AoS_second_order(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            coarseData,
            fineDataSecondOrder
          );
          for (int n = 0; n < faceSize; n++)
            validateNumericalEqualsWithEpsWithParams3(
              fineDataTensorProduct[n],
              fineDataSecondOrder[n],
              0.01,
              faceMarker1.getRelativePositionWithinFatherFace(),
              axis,
              n
            );

          for (int n = 0; n < faceSize; n++)
            fineDataTensorProduct[n] = 0;
          for (int n = 0; n < faceSize; n++)
            fineDataSecondOrder[n] = 0;

          interpolateHaloLayer_AoS_tensor_product(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            normalInterp,
            tangentInterp,
            coarseData,
            fineDataTensorProduct
          );
          interpolateHaloLayer_AoS_second_order(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            coarseData,
            fineDataSecondOrder
          );
          for (int n = 0; n < faceSize; n++)
            validateNumericalEqualsWithEpsWithParams3(
              fineDataTensorProduct[n],
              fineDataSecondOrder[n],
              0.01,
              faceMarker1.getRelativePositionWithinFatherFace(),
              axis + 3,
              n
            );
        }
      }
    }
  }
}

void toolbox::blockstructured::tests::IandRSecondOrderTest::compareRestrictLinearCaseToTensorProduct(){
  const double normalRestrict[] = {
    0.0000, 0.0000, 0.0000, 0.333333, 0.333333, 0.333333,
    0.0000, 0.0000, 0.0000, 0.0000, -2.0000, 3.0000,
    0.0000, 0.0000, 0.0000, 0.0000, -5.0000, 6.0000
  };

  const double tangentRestrict[] = {
    0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000
  };

  constexpr int patchSize = 5;
  constexpr int overlap   = 3;
  constexpr int unknowns  = 59;
  const int     faceSize  = unknowns * patchSize * patchSize * overlap * 2;

  double fineData[faceSize];
  double coarseDataTensorProduct[faceSize];
  double coarseDataSecondOrder[faceSize];

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        peano4::grid::GridTraversalEvent dummyEvent;
        dummyEvent.setRelativePositionToFather(
          tarch::la::Vector<Dimensions, int>{i, j, k}
        );

        for (int axis = 0; axis < 3; axis++) {
          peano4::datamanagement::FaceMarker faceMarker1(dummyEvent, axis);
          peano4::datamanagement::FaceMarker
            faceMarker2(dummyEvent, axis + Dimensions);

          for (int n = 0; n < faceSize; n++)
            fineData[n] = static_cast<double>(n + 1) / 16.0;
          for (int n = 0; n < faceSize; n++)
            coarseDataTensorProduct[n] = 0;
          for (int n = 0; n < faceSize; n++)
            coarseDataSecondOrder[n] = 0;

          restrictHaloLayer_AoS_tensor_product(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            normalRestrict,
            tangentRestrict,
            fineData,
            coarseDataTensorProduct
          );
          restrictHaloLayer_AoS_second_order(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            fineData,
            coarseDataSecondOrder
          );
          for (int n = 0; n < faceSize; n++)
            validateNumericalEqualsWithEpsWithParams3(
              coarseDataTensorProduct[n],
              coarseDataSecondOrder[n],
              0.01,
              faceMarker1.getRelativePositionWithinFatherFace(),
              axis,
              n
            );

          for (int n = 0; n < faceSize; n++)
            coarseDataTensorProduct[n] = 0;
          for (int n = 0; n < faceSize; n++)
            coarseDataSecondOrder[n] = 0;

          restrictHaloLayer_AoS_tensor_product(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            normalRestrict,
            tangentRestrict,
            fineData,
            coarseDataTensorProduct
          );
          restrictHaloLayer_AoS_second_order(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            fineData,
            coarseDataSecondOrder
          );
          for (int n = 0; n < faceSize; n++)
            validateNumericalEqualsWithEpsWithParams3(
              coarseDataTensorProduct[n],
              coarseDataSecondOrder[n],
              0.01,
              faceMarker1.getRelativePositionWithinFatherFace(),
              axis + 3,
              n
            );
        }
      }
    }
  }
}
