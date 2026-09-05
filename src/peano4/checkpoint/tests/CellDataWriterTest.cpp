// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellDataWriterTest.h"

#ifdef PEANO_WITH_HDF5
#include "peano4/checkpoint/CellDataWriter.h"
#endif

peano4::checkpoint::tests::CellDataWriterTest::CellDataWriterTest() :
  TestCase("peano4::checkpoint::tests::CellDataWriterTest") {}

void peano4::checkpoint::tests::CellDataWriterTest::testAppendWidensEverySourceType() {
#ifdef PEANO_WITH_HDF5
  const tarch::la::Vector<DIMENSIONS, int> latticeIndex(1);
  const float                              floatValues[2]      = {0.5F, -2.0F};
  const double                             doubleValues[2]     = {0.25, -4.0};
  const long double                        longDoubleValues[2] = {0.125L, -8.0L};

  peano4::checkpoint::CellDataWriter::Records records;
  records.append(1, latticeIndex, floatValues, 2);
  records.append(1, latticeIndex, doubleValues, 2);
  records.append(1, latticeIndex, longDoubleValues, 2);

  validateEquals(records.levels.size(), 3);
  validateEquals(records.keys.size(), 3);
  validateEquals(records.values.size(), 6);
  validateNumericalEquals(records.values[0], 0.5);
  validateNumericalEquals(records.values[1], -2.0);
  validateNumericalEquals(records.values[2], 0.25);
  validateNumericalEquals(records.values[3], -4.0);
  validateNumericalEquals(records.values[4], 0.125);
  validateNumericalEquals(records.values[5], -8.0);

  records.clear();
  validate(records.values.empty());
#endif
}

void peano4::checkpoint::tests::CellDataWriterTest::run() { testMethod(testAppendWidensEverySourceType); }
