// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class FormatTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::FormatTest final : public tarch::tests::TestCase {
public:
  FormatTest();
  FormatTest(const FormatTest&)            = delete;
  FormatTest(FormatTest&&)                 = delete;
  FormatTest& operator=(const FormatTest&) = delete;
  FormatTest& operator=(FormatTest&&)      = delete;
  ~FormatTest() override                   = default;

  void run() override;

private:
  void testEveryNameParsesToItsFormatAndBack();
  void testTheDefaultFormatIsAvailable();
  void testTheWritersOfAnAvailableFormatExist();
};
