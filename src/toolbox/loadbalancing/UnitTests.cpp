// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "toolbox/loadbalancing/tests/BlacklistTest.h"
#include "toolbox/loadbalancing/tests/InitialSplitScenarioTest.h"
#include "toolbox/loadbalancing/tests/LoadBalancerTest.h"
#include "toolbox/loadbalancing/tests/StatisticsTest.h"

tarch::tests::TestCase* toolbox::loadbalancing::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new toolbox::loadbalancing::tests::BlacklistTest());
  result->addTestCase(new toolbox::loadbalancing::tests::InitialSplitScenarioTest());
  result->addTestCase(new toolbox::loadbalancing::tests::LoadBalancerTest());
  result->addTestCase(new toolbox::loadbalancing::tests::StatisticsTest());

  return result;
}
