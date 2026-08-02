// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "toolbox/blockstructured/tests/CurviTest.h"
#include "toolbox/blockstructured/tests/EnumerationTest.h"
#include "toolbox/blockstructured/tests/InterpolationTest.h"
#include "toolbox/blockstructured/tests/MultiresolutionDetailTest.h"
#include "toolbox/blockstructured/tests/RestrictionTest.h"

tarch::tests::TestCase* toolbox::blockstructured::getUnitTests() {
  auto* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new toolbox::blockstructured::tests::CurviTest());
  result->addTestCase(new toolbox::blockstructured::tests::EnumerationTest());
  result->addTestCase(new toolbox::blockstructured::tests::InterpolationTest());
  result->addTestCase(new toolbox::blockstructured::tests::MultiresolutionDetailTest());
  result->addTestCase(new toolbox::blockstructured::tests::RestrictionTest());

  return result;
}
