// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class CurviTracerTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::CurviTracerTest final : public tarch::tests::TestCase {
public:
  CurviTracerTest();
  CurviTracerTest(const CurviTracerTest&)            = delete;
  CurviTracerTest(CurviTracerTest&&)                 = delete;
  CurviTracerTest& operator=(const CurviTracerTest&) = delete;
  CurviTracerTest& operator=(CurviTracerTest&&)      = delete;
  ~CurviTracerTest() override                        = default;

  void run() override;

private:
  void testInvertCoordinatesAppendsEveryMappedPosition();
  void testIsFaultFaceSelectsTheFaceOnTheFault();
  void testRecordsTheSolutionAtEveryCoordinateOfTheCell();
  void testRecordsTheFaceAverageAndTheJumpOfTheVectorComponents();
};
