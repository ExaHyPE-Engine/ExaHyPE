// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::dataview::tests {
  class EnumeratorTest;
} // namespace peano4::dataview::tests

class peano4::dataview::tests::EnumeratorTest final : public tarch::tests::TestCase {
public:
  EnumeratorTest();

  void run() override;

private:
  void testVertexEnumeratorFollowsItsSlots();
  void testFaceEnumeratorFollowsItsSlots();
  void testEnumeratorWritesThrough();
  void testPositionWithoutEntityReadsTheSentinel();
};
