// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::blockstructured::tests {
  class RestrictionTest;
} // namespace toolbox::blockstructured::tests

class toolbox::blockstructured::tests::RestrictionTest final : public tarch::tests::TestCase {
private:
  void testRestrictHaloLayer();
  void testRestrictCell();

public:
  RestrictionTest();
  ~RestrictionTest() override = default;

  void run() override;
};
