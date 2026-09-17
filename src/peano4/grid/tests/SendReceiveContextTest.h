// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class SendReceiveContextTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::SendReceiveContextTest final : public tarch::tests::TestCase {
public:
  SendReceiveContextTest();

  void run() override;

private:
  void testTheConsolidationContextPicksThePathTheElementDeclares();
};
