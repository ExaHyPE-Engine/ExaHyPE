// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ChildDataStageTest.h"

#include "exahype2/amr/ChildDataStage.h"

exahype2::amr::tests::ChildDataStageTest::ChildDataStageTest() :
  TestCase("exahype2::amr::tests::ChildDataStageTest") {}

void exahype2::amr::tests::ChildDataStageTest::run() {
  testMethod(testSiblingSetLifecycle);
  testMethod(testNestedFrames);
  testMethod(testPartialFrameAccessors);
  testMethod(testClaimedFramesCarryTheirTag);
}

void exahype2::amr::tests::ChildDataStageTest::testSiblingSetLifecycle() {
  exahype2::amr::ChildDataStage<double> stage(4, 2);
  const int                             coarseKey = 0;

  const double childValues[4][2] = {{0.0, 0.5}, {1.0, 1.5}, {2.0, 2.5}, {3.0, 3.5}};

  // Arrival order must not matter and only the last arrival completes the set.
  validate(not stage.stage(&coarseKey, 2, childValues[2]));
  validate(not stage.stage(&coarseKey, 0, childValues[0]));
  validate(not stage.stage(&coarseKey, 3, childValues[3]));
  validate(stage.stage(&coarseKey, 1, childValues[1]));

  for (int child = 0; child < 4; child++) {
    validateNumericalEquals(stage.child(&coarseKey, child)[0], childValues[child][0]);
    validateNumericalEquals(stage.child(&coarseKey, child)[1], childValues[child][1]);
  }

  stage.discard(&coarseKey);
  validate(not stage.hasFrame(&coarseKey));
}

void exahype2::amr::tests::ChildDataStageTest::testNestedFrames() {
  exahype2::amr::ChildDataStage<double> stage(2, 1);
  const int                             outerKey = 0;
  const int                             innerKey = 1;

  const double outerFirst[1]  = {10.0};
  const double outerSecond[1] = {11.0};
  const double innerFirst[1]  = {20.0};
  const double innerSecond[1] = {21.0};

  // An inner sibling set opens and completes while the outer one is in flight,
  // as it does when several levels collapse within one traversal.
  validate(not stage.stage(&outerKey, 0, outerFirst));
  validate(not stage.stage(&innerKey, 0, innerFirst));
  validate(stage.stage(&innerKey, 1, innerSecond));
  validateNumericalEquals(stage.child(&innerKey, 0)[0], 20.0);
  validateNumericalEquals(stage.child(&innerKey, 1)[0], 21.0);
  stage.discard(&innerKey);

  validate(stage.stage(&outerKey, 1, outerSecond));
  validateNumericalEquals(stage.child(&outerKey, 0)[0], 10.0);
  validateNumericalEquals(stage.child(&outerKey, 1)[0], 11.0);
  stage.discard(&outerKey);
}

void exahype2::amr::tests::ChildDataStageTest::testPartialFrameAccessors() {
  exahype2::amr::ChildDataStage<double> stage(3, 1);
  const int                             coarseKey  = 0;
  const int                             unknownKey = 1;

  const double values[1] = {7.0};
  validate(not stage.hasFrame(&coarseKey));
  validate(not stage.stage(&coarseKey, 1, values));
  validate(stage.hasFrame(&coarseKey));
  validate(not stage.hasFrame(&unknownKey));
  validate(not stage.hasChild(&coarseKey, 0));
  validate(stage.hasChild(&coarseKey, 1));
  validate(not stage.hasChild(&coarseKey, 2));
  validateNumericalEquals(stage.child(&coarseKey, 1)[0], 7.0);
  stage.discard(&coarseKey);
}

void exahype2::amr::tests::ChildDataStageTest::testClaimedFramesCarryTheirTag() {
  exahype2::amr::ChildDataStage<double> stage(3, 2);
  const int                             coarseKey = 0;
  constexpr int                         Side      = 1;

  // The frame carries the side a restriction targets alongside the claimed values.
  double* const firstChild = stage.claim(&coarseKey, 0, Side);
  firstChild[0]            = 4.0;
  firstChild[1]            = 4.5;
  validateEquals(stage.frameTag(&coarseKey), Side);
  validate(stage.hasChild(&coarseKey, 0));
  validate(not stage.hasChild(&coarseKey, 1));

  // A child another rank holds never claims, so the gap between the claims has
  // to survive until the reduction reads the frame.
  double* const lastChild = stage.claim(&coarseKey, 2, Side);
  lastChild[0]            = 6.0;
  lastChild[1]            = 6.5;
  validateEquals(stage.frameTag(&coarseKey), Side);
  validate(not stage.hasChild(&coarseKey, 1));

  validateNumericalEquals(stage.child(&coarseKey, 0)[0], 4.0);
  validateNumericalEquals(stage.child(&coarseKey, 0)[1], 4.5);
  validateNumericalEquals(stage.child(&coarseKey, 2)[0], 6.0);
  validateNumericalEquals(stage.child(&coarseKey, 2)[1], 6.5);

  stage.discard(&coarseKey);
  validate(not stage.hasFrame(&coarseKey));
}
