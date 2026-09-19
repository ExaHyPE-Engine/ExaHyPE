// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class CoordinateOwnershipTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::CoordinateOwnershipTest final : public tarch::tests::TestCase {
public:
  CoordinateOwnershipTest();
  CoordinateOwnershipTest(const CoordinateOwnershipTest&)            = delete;
  CoordinateOwnershipTest(CoordinateOwnershipTest&&)                 = delete;
  CoordinateOwnershipTest& operator=(const CoordinateOwnershipTest&) = delete;
  CoordinateOwnershipTest& operator=(CoordinateOwnershipTest&&)      = delete;
  ~CoordinateOwnershipTest() override                                = default;

  void run() override;

private:
  void testTheLowerCellOwnsACoordinateBetweenTwoCells();
  void testTheCellOnTheLowerDomainWallOwnsTheWall();
};
