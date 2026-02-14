// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "tests/BlacklistTest.h"
#include "tests/LoadBalancerTest.h"
#include "tests/StatisticsTest.h"

tarch::tests::TestCase* toolbox::loadbalancing::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new tests::BlacklistTest());
  result->addTestCase(new tests::LoadBalancerTest());
  result->addTestCase(new tests::StatisticsTest());

  return result;
}
