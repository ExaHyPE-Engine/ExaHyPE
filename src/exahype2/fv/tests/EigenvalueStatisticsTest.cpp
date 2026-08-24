// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EigenvalueStatisticsTest.h"

#include "exahype2/fv/EigenvalueStatistics.h"

exahype2::fv::tests::EigenvalueStatisticsTest::EigenvalueStatisticsTest() :
  tarch::tests::TestCase("exahype2::fv::tests::EigenvalueStatisticsTest") {}

void exahype2::fv::tests::EigenvalueStatisticsTest::run() {
  testMethod(testAccumulation);
#if PEANO_ASSERTIONS == 0
  testMethod(testInvalidEigenvalueAccumulation);
#endif
}

void exahype2::fv::tests::EigenvalueStatisticsTest::testAccumulation() {
  double maximum = 0.0;
  exahype2::fv::accumulateMaxEigenvalue(maximum, 3.0);
  exahype2::fv::accumulateMaxEigenvalue(maximum, 2.0);
  exahype2::fv::accumulateMaxEigenvalue(maximum, 0.0);
  validateNumericalEquals(maximum, 3.0);

  double mixedPrecisionMaximum = 0.0;
  exahype2::fv::accumulateMaxEigenvalue(mixedPrecisionMaximum, 2.5F);
  validateNumericalEquals(mixedPrecisionMaximum, 2.5);
}

void exahype2::fv::tests::EigenvalueStatisticsTest::testInvalidEigenvalueAccumulation() {
  double negativeMaximum = 3.0;
  exahype2::fv::accumulateMaxEigenvalue(negativeMaximum, -2.0);
  exahype2::fv::accumulateMaxEigenvalue(negativeMaximum, 5.0);
  validateEquals(negativeMaximum, -2.0);

  constexpr auto PositiveInfinity = std::numeric_limits<double>::infinity();
  double         infiniteMaximum  = 0.0;
  exahype2::fv::accumulateMaxEigenvalue(infiniteMaximum, PositiveInfinity);
  exahype2::fv::accumulateMaxEigenvalue(infiniteMaximum, 5.0);
  validateEquals(infiniteMaximum, PositiveInfinity);
}
