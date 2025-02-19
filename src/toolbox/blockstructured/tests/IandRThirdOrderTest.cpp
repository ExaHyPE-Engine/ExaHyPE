#include "IandRThirdOrderTest.h"

#include "../Interpolation.h"
#include "../Restriction.h"

toolbox::blockstructured::tests::IandRThirdOrderTest::IandRThirdOrderTest():
  TestCase("toolbox::blockstructured::tests::IandRThirdOrderTest") {}

void toolbox::blockstructured::tests::IandRThirdOrderTest::run() {
  testMethod(compareInterpolateLinearCaseToTensorProduct);
  testMethod(compareRestrictLinearCaseToTensorProduct);
}

void toolbox::blockstructured::tests::IandRThirdOrderTest::
  compareInterpolateLinearCaseToTensorProduct() {

  double normalInterp[] = {
    0.0000,
    0.3333333,
    0.6666667,
    0.0000,
    0.0000,
    0.0000,
    0.0000,
    0.0000,
    1.0000,
    0.0000,
    0.0000,
    0.0000,
    0.0000,
    0.0000,
    0.6666667,
    0.3333333,
    0.0000,
    0.0000};

  double tangentInterp[] = {
    1.3333333, -0.3333333, 0.0000,    0.0000,    0.0000,    1.0000,
    0.0000,    0.0000,     0.0000,    0.0000,    0.6666667, 0.3333333,
    0.0000,    0.0000,     0.0000,    0.3333333, 0.6666667, 0.0000,
    0.0000,    0.0000,     0.0000,    1.0000,    0.0000,    0.0000,
    0.0000,    0.0000,     0.6666667, 0.3333333, 0.0000,    0.0000,
    0.0000,    0.3333333,  0.6666667, 0.0000,    0.0000,    0.0000,
    0.0000,    1.0000,     0.0000,    0.0000,    0.0000,    0.0000,
    0.6666667, 0.3333333,  0.0000,    0.0000,    0.0000,    0.3333333,
    0.6666667, 0.0000,     0.0000,    0.0000,    0.0000,    1.0000,
    0.0000,    0.0000,     0.0000,    0.0000,    0.6666667, 0.3333333,
    0.0000,    0.0000,     0.0000,    0.3333333, 0.6666667, 0.0000,
    0.0000,    0.0000,     0.0000,    1.0000,    0.0000,    0.0000,
    0.0000,    -0.3333333, 1.3333333};

  constexpr int patchSize = 5;
  constexpr int overlap   = 3;
  constexpr int unknowns  = 59;
  const int     faceSize  = unknowns * patchSize * patchSize * overlap * 2;

  double coarseData[faceSize];
  double fineDataTensorProduct[faceSize];
  double fineDataThirdOrder[faceSize];

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        peano4::grid::GridTraversalEvent dummyEvent;
        dummyEvent.setRelativePositionToFather(
          tarch::la::Vector<Dimensions, int>{i, j, k}
        );

        for (int axis = 0; axis < 3; axis++) {
          peano4::datamanagement::FaceMarker faceMarker1(dummyEvent, axis);
          peano4::datamanagement::FaceMarker faceMarker2(dummyEvent, axis + 3);

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            coarseData[n] = (double)(n) / 16.0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            fineDataTensorProduct[n] = 0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            fineDataThirdOrder[n] = 0;

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
          interpolateHaloLayer_AoS_third_order(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            coarseData,
            fineDataThirdOrder
          );

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            validateNumericalEqualsWithEpsWithParams3(
              fineDataTensorProduct[n],
              fineDataThirdOrder[n],
              0.01,
              faceMarker1.getRelativePositionWithinFatherFace(),
              axis,
              n
            );

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            fineDataTensorProduct[n] = 0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            fineDataThirdOrder[n] = 0;

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
          interpolateHaloLayer_AoS_third_order(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            coarseData,
            fineDataThirdOrder
          );

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            validateNumericalEqualsWithEpsWithParams3(
              fineDataTensorProduct[n],
              fineDataThirdOrder[n],
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

void toolbox::blockstructured::tests::IandRThirdOrderTest::
  compareRestrictLinearCaseToTensorProduct() {

  const double normalRestrict[] = {
    0.0000,
    0.0000,
    0.0000,
    0.333333,
    0.333333,
    0.333333,
    0.0000,
    0.0000,
    0.0000,
    0.0000,
    -2.0000,
    3.0000,
    0.0000,
    0.0000,
    0.0000,
    0.0000,
    -5.0000,
    6.0000};

  const double tangentRestrict[] = {
    0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 1.0000, 0.0000};

  constexpr int patchSize = 5;
  constexpr int overlap   = 3;
  constexpr int unknowns  = 59;
  const int     faceSize  = unknowns * patchSize * patchSize * overlap * 2;

  double fineData[faceSize];
  double coarseDataTensorProduct[faceSize];
  double coarseDataThirdOrder[faceSize];

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        peano4::grid::GridTraversalEvent dummyEvent;
        dummyEvent.setRelativePositionToFather(
          tarch::la::Vector<Dimensions, int>{i, j, k}
        );

        for (int axis = 0; axis < 3; axis++) {
          peano4::datamanagement::FaceMarker faceMarker1(dummyEvent, axis);
          peano4::datamanagement::FaceMarker faceMarker2(dummyEvent, axis + 3);

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            fineData[n] = (double)(n) / 16.0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            coarseDataTensorProduct[n] = 0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            coarseDataThirdOrder[n] = 0;

          restrictInnerHalfOfHaloLayer_AoS_tensor_product(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            normalRestrict,
            tangentRestrict,
            fineData,
            coarseDataTensorProduct,
            true
          );
          restrictInnerHalfOfHaloLayer_AoS_third_order(
            faceMarker1,
            patchSize,
            overlap,
            unknowns,
            fineData,
            coarseDataThirdOrder,
            true
          );

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            validateNumericalEqualsWithEpsWithParams3(
              coarseDataTensorProduct[n],
              coarseDataThirdOrder[n],
              0.001,
              faceMarker1.getRelativePositionWithinFatherFace(),
              axis,
              n
            );

          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            coarseDataTensorProduct[n] = 0;
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            coarseDataThirdOrder[n] = 0;

          restrictInnerHalfOfHaloLayer_AoS_tensor_product(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            normalRestrict,
            tangentRestrict,
            fineData,
            coarseDataTensorProduct,
            true
          );
          restrictInnerHalfOfHaloLayer_AoS_third_order(
            faceMarker2,
            patchSize,
            overlap,
            unknowns,
            fineData,
            coarseDataThirdOrder,
            true
          );
          for (int n = 0; n < unknowns * patchSize * patchSize * overlap * 2;
               n++)
            validateNumericalEqualsWithEpsWithParams3(
              coarseDataTensorProduct[n],
              coarseDataThirdOrder[n],
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
