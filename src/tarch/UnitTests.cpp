// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "accelerator/tests/MemoryManagerTest.h"
#include "la/tests/DynamicMatrixTest.h"
#include "la/tests/GramSchmidtTest.h"
#include "la/tests/LUDecompositionTest.h"
#include "la/tests/MatrixTest.h"
#include "la/tests/MatrixVectorTest.h"
#include "la/tests/ScalarTest.h"
#include "la/tests/VectorTest.h"
#include "mpi/tests/StringTest.h"
#include "multicore/tests/SpawnTaskTest.h"
#include "tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* tarch::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tests::TreeTestCaseCollection();

  result->addTestCase(new la::tests::GramSchmidtTest());
  result->addTestCase(new la::tests::LUDecompositionTest());
  result->addTestCase(new la::tests::MatrixTest());
  result->addTestCase(new la::tests::MatrixVectorTest());
  result->addTestCase(new la::tests::ScalarTest());
  result->addTestCase(new la::tests::VectorTest());
  result->addTestCase(new la::tests::DynamicMatrixTest());

  result->addTestCase(new mpi::tests::StringTest());

  result->addTestCase(new accelerator::tests::MemoryManagerTest());

  result->addTestCase(new multicore::tests::SpawnTaskTest());

  return result;
}
