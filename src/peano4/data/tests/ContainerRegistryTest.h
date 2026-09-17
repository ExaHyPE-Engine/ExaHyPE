// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::data::tests {
  class ContainerRegistryTest;
} // namespace peano4::data::tests

class peano4::data::tests::ContainerRegistryTest final : public tarch::tests::TestCase {
public:
  ContainerRegistryTest();
  ContainerRegistryTest(const ContainerRegistryTest&)            = delete;
  ContainerRegistryTest(ContainerRegistryTest&&)                 = delete;
  ContainerRegistryTest& operator=(const ContainerRegistryTest&) = delete;
  ContainerRegistryTest& operator=(ContainerRegistryTest&&)      = delete;
  ~ContainerRegistryTest() override                              = default;

  void run() override;

private:
  void testContainersAreVisitedInContainerIdOrder();
  void testEveryOperationReachesEveryContainerOnce();
  void testSubtreeTaskReplicationNeedsEveryContainer();
  void testDetachedContainersAreNoLongerVisited();
  void testEveryOperationForwardsItsArgumentsUnchanged();
  void testAllocatePayloadPoolsBindsEveryContainer();
};
