// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileWriterTest.h"

#include "tarch/plotter/FileWriter.h"

tarch::plotter::tests::FileWriterTest::FileWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::FileWriterTest") {}

void
tarch::plotter::tests::FileWriterTest::run() {
  testMethod(testTheTimeStampReadsBackExactly);
}

/**
 * Every time stamp reads back as the same double.
 */
void
tarch::plotter::tests::FileWriterTest::testTheTimeStampReadsBackExactly() {
  const double times[] = {0.0, 0.1, 1.0 / 3.0, 1000000.25, 1000000.5, 1.0e-300, 12345.678901234567};
  for (const double time : times) {
    const std::string timeStamp = tarch::plotter::getExactTimeStamp(time);
    validateEqualsWithParams1(std::stod(timeStamp), time, timeStamp);
  }
  validateEquals(tarch::plotter::getExactTimeStamp(0.1), std::string("0.1"));
  validate(tarch::plotter::getExactTimeStamp(1000000.25) != tarch::plotter::getExactTimeStamp(1000000.5));
}
