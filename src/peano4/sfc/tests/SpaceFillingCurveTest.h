// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::sfc::tests {
  class SpaceFillingCurveTest;
} // namespace peano4::sfc::tests

class peano4::sfc::tests::SpaceFillingCurveTest final : public tarch::tests::TestCase {
public:
  SpaceFillingCurveTest();
  SpaceFillingCurveTest(const SpaceFillingCurveTest&)            = delete;
  SpaceFillingCurveTest(SpaceFillingCurveTest&&)                 = delete;
  SpaceFillingCurveTest& operator=(const SpaceFillingCurveTest&) = delete;
  SpaceFillingCurveTest& operator=(SpaceFillingCurveTest&&)      = delete;
  ~SpaceFillingCurveTest() override                              = default;

  void run() override;

private:
  template <class Curve>
  void validateChildrenMatchTheDerivedTable();

  template <class Curve>
  void validateLeafSequenceIsAHamiltonianPath();

  template <class Curve>
  void validateCurveParameterFollowsTheLeafOrder();

  void testChildrenMatchTheDerivedTable();
  void testLeafSequenceIsAHamiltonianPath();
  void testCurveParameterFollowsTheLeafOrder();
  void testPeanoMatchesTheSerpentineClosedForm();
  void testHilbertVisitsTheTextbookOrder();
};
