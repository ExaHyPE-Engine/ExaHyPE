// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EigenvalueStatisticsTest.h"

#include "exahype2/dg/EigenvalueStatistics.h"

exahype2::dg::tests::EigenvalueStatisticsTest::EigenvalueStatisticsTest() :
  tarch::tests::TestCase("exahype2::dg::tests::EigenvalueStatisticsTest") {}

void exahype2::dg::tests::EigenvalueStatisticsTest::run() {
  testMethod(testAnisotropicAccumulation);
#if PEANO_ASSERTIONS == 0
  testMethod(testInvalidEigenvalueAccumulation);
#endif
}

void exahype2::dg::tests::EigenvalueStatisticsTest::testAnisotropicAccumulation() {
#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> cellSize{2.0, 4.0};
  constexpr std::array<double, DIMENSIONS>    FirstPhysicalEigenvalues{3.0, 5.0};
  constexpr std::array<double, DIMENSIONS>    SecondPhysicalEigenvalues{7.0, 1.0};
  constexpr double                            FirstScaledEigenvalue  = 2.75;
  constexpr double                            SecondScaledEigenvalue = 3.75;
#else
  const tarch::la::Vector<DIMENSIONS, double> cellSize{2.0, 4.0, 8.0};
  constexpr std::array<double, DIMENSIONS>    FirstPhysicalEigenvalues{3.0, 5.0, 2.0};
  constexpr std::array<double, DIMENSIONS>    SecondPhysicalEigenvalues{7.0, 1.0, 4.0};
  constexpr double                            FirstScaledEigenvalue  = 3.0;
  constexpr double                            SecondScaledEigenvalue = 4.25;
#endif

  exahype2::dg::MaxEigenvalueStatistics statistics;
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, 0.0);

  statistics.accumulate(cellSize, [&](const int dimension) { return FirstPhysicalEigenvalues[dimension]; });
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 5.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, FirstScaledEigenvalue);

  statistics.accumulate(cellSize, [&](const int dimension) { return SecondPhysicalEigenvalues[dimension]; });
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, 7.0);
  validateNumericalEquals(statistics.maxScaledEigenvalue, SecondScaledEigenvalue);

  exahype2::dg::MaxEigenvalueStatistics stationaryStatistics;
  stationaryStatistics.accumulate(cellSize, []([[maybe_unused]] const int dimension) { return 0.0; });
  validateNumericalEquals(stationaryStatistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(stationaryStatistics.maxScaledEigenvalue, 0.0);
}

void exahype2::dg::tests::EigenvalueStatisticsTest::testInvalidEigenvalueAccumulation() {
#if DIMENSIONS == 2
  const tarch::la::Vector<DIMENSIONS, double> cellSize{1.0, 1.0};
#else
  const tarch::la::Vector<DIMENSIONS, double> cellSize{1.0, 1.0, 1.0};
#endif

  exahype2::dg::MaxEigenvalueStatistics negativeStatistics;
  negativeStatistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? -2.0 : 5.0; });
  validateEquals(negativeStatistics.maxPhysicalEigenvalue, -2.0);
  validateEquals(negativeStatistics.maxScaledEigenvalue, -2.0);

  constexpr auto                        PositiveInfinity = std::numeric_limits<double>::infinity();
  exahype2::dg::MaxEigenvalueStatistics infiniteStatistics;
  infiniteStatistics.accumulate(cellSize, [](const int dimension) { return dimension == 0 ? PositiveInfinity : 5.0; });
  validateEquals(infiniteStatistics.maxPhysicalEigenvalue, PositiveInfinity);
  validateEquals(infiniteStatistics.maxScaledEigenvalue, PositiveInfinity);
}
