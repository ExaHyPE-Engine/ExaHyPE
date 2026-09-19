// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class ForEachSubcellTest;
} // namespace exahype2::dg::limiting::tests

class exahype2::dg::limiting::tests::ForEachSubcellTest final : public tarch::tests::TestCase {
public:
  ForEachSubcellTest();
  ForEachSubcellTest(const ForEachSubcellTest&)            = delete;
  ForEachSubcellTest(ForEachSubcellTest&&)                 = delete;
  ForEachSubcellTest& operator=(const ForEachSubcellTest&) = delete;
  ForEachSubcellTest& operator=(ForEachSubcellTest&&)      = delete;
  ~ForEachSubcellTest() override                           = default;

  void run() override;

private:
  void testVisitsEverySubcellAtItsPatchCentre();
};
