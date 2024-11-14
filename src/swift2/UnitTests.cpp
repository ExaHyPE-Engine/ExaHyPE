#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* swift2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  return result;
}
