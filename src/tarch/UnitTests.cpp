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
#include "plotter/tests/HDF5FileWriterTest.h"
#include "plotter/tests/NetCDFFileWriterTest.h"
#include "plotter/tests/VTUFileWriterTest.h"
#include "reader/tests/JSONReaderTest.h"
#include "reader/tests/NetCDFFieldSamplerTest.h"
#include "reader/tests/NetCDFFileReaderTest.h"
#include "tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* tarch::getUnitTests() {
  tests::TreeTestCaseCollection* result = new tests::TreeTestCaseCollection();

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

  result->addTestCase(new plotter::tests::HDF5FileWriterTest());
  result->addTestCase(new plotter::tests::NetCDFFileWriterTest());
  result->addTestCase(new plotter::tests::VTUFileWriterTest());
  result->addTestCase(new reader::tests::JSONReaderTest());
  result->addTestCase(new reader::tests::NetCDFFileReaderTest());
  result->addTestCase(new reader::tests::NetCDFFieldSamplerTest());

  return result;
}
