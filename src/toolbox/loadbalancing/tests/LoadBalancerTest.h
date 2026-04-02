// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace loadbalancing {
    namespace tests {
      class LoadBalancerTest;
    } // namespace tests
  } // namespace loadbalancing
} // namespace toolbox

class toolbox::loadbalancing::tests::LoadBalancerTest final: public tarch::tests::TestCase {
private:
  void testEnableDisableAndConfigurationHook();
  void testHasSplitRecentlyThreshold();
  void testLocalLoadBalancingString();

public:
  LoadBalancerTest();
  ~LoadBalancerTest() override final = default;

  void run() override final;
};
