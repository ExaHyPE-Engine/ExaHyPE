// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::plotter::tests {
  class PatchPlotterTest;
} // namespace exahype2::plotter::tests

class exahype2::plotter::tests::PatchPlotterTest final : public tarch::tests::TestCase {
public:
  PatchPlotterTest();
  PatchPlotterTest(const PatchPlotterTest&)            = delete;
  PatchPlotterTest(PatchPlotterTest&&)                 = delete;
  PatchPlotterTest& operator=(const PatchPlotterTest&) = delete;
  PatchPlotterTest& operator=(PatchPlotterTest&&)      = delete;
  ~PatchPlotterTest() override                         = default;

  void run() override;

private:
  void testNodalPatchPlotsOnePointPerNode();
  void testNodalPatchReadsTheAlternativePatch();
  void testCellPatchPlotsOneCellPerVolume();
  void testCellPatchRunsTheCellDataPlotsOfTheCaller();
  void testNodalPatchRunsTheCellDataPlotsOfTheCaller();
};
