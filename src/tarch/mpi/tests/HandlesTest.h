// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class HandlesTest;
} // namespace tarch::mpi::tests

/**
 * Checks that the handle wrappers split, duplicate and free communicators, report ranks and
 * attributes, create and free info objects, and build a datatype and a reduction operation that a
 * collective reduces with.
 */
class tarch::mpi::tests::HandlesTest final : public tarch::tests::TestCase {
public:
  HandlesTest();
  HandlesTest(const HandlesTest& other)            = delete;
  HandlesTest(HandlesTest&& other)                 = delete;
  HandlesTest& operator=(const HandlesTest& other) = delete;
  HandlesTest& operator=(HandlesTest&& other)      = delete;
  ~HandlesTest() override                          = default;

  void run() override;

private:
  void testASplitByParityNumbersTheRanksOfEachHalf();
  void testADuplicateContainsTheSameRanksAndCommunicates();
  void testTheTagUpperBoundIsAnAttribute();
  void testAnInfoObjectIsCreatedAndFreed();
  void testARecordTypeAndOperationReduceRecords();
};
