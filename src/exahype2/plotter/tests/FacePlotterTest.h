// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::plotter::tests {
  class FacePlotterTest;
} // namespace exahype2::plotter::tests

class exahype2::plotter::tests::FacePlotterTest final : public tarch::tests::TestCase {
public:
  FacePlotterTest();
  FacePlotterTest(const FacePlotterTest&)            = delete;
  FacePlotterTest(FacePlotterTest&&)                 = delete;
  FacePlotterTest& operator=(const FacePlotterTest&) = delete;
  FacePlotterTest& operator=(FacePlotterTest&&)      = delete;
  ~FacePlotterTest() override                        = default;

  void run() override;

private:
  void testEverySelectedFaceBecomesItsNodesAndBothTraces();
};
