// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "runner/tests/FormatTest.h"
#include "runner/tests/GridSnapshotTest.h"
#include "runner/tests/RuntimeParametersTest.h"
#include "runner/tests/TableTest.h"
#include "runner/tests/TextLayoutTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* runner::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();
  result->addTestCase(new runner::tests::FormatTest());
  result->addTestCase(new runner::tests::GridSnapshotTest());
  result->addTestCase(new runner::tests::RuntimeParametersTest());
  result->addTestCase(new runner::tests::TableTest());
  result->addTestCase(new runner::tests::TextLayoutTest());
  return result;
}
