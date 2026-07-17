// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::loadbalancing::tests {
  class LoadBalancerTest;
} // namespace toolbox::loadbalancing::tests

class toolbox::loadbalancing::tests::LoadBalancerTest final : public tarch::tests::TestCase {
public:
  LoadBalancerTest();
  void run() override;

private:
  void testEnableDisableAndConfigurationHook();
  void testStationaryLevelWaitsOnlyWhenSplitCanStart();
  void testDescribeLocalDecomposition();
  void testInitialTargetIsStateIndependent();
  void testSettledInitialSplitStops();
  void testNullConfigurationIsRejected();
  void testStateOutput();
  void testUniformGridCompleteFlag();
};
