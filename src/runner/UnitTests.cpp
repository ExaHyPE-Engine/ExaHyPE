// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "tests/FormatTest.h"
#include "tests/TableTest.h"
#include "tests/TextLayoutTest.h"

tarch::tests::TestCase* runner::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();
  result->addTestCase(new tests::FormatTest());
  result->addTestCase(new tests::TableTest());
  result->addTestCase(new tests::TextLayoutTest());
  return result;
}
