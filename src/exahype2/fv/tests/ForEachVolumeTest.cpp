// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ForEachVolumeTest.h"

#include "exahype2/fv/ForEachVolume.h"
#include "peano4/Constants.h"

namespace {
  constexpr int    VolumesPerAxis = 3;
  constexpr int    Values         = 2;
  constexpr int    Volumes        = peano4::power(VolumesPerAxis, DIMENSIONS);
  constexpr double CellWidth      = 3.0;
} // namespace

exahype2::fv::tests::ForEachVolumeTest::ForEachVolumeTest() :
  tarch::tests::TestCase("exahype2::fv::tests::ForEachVolumeTest") {}

void exahype2::fv::tests::ForEachVolumeTest::run() {
  testMethod(testVisitsEveryVolumeInLinearisedOrder);
  testMethod(testWritesThroughTheValuePointer);
}

/**
 * Checks that the loop reaches every volume once in linearised order and hands each one its
 * value pointer, its centre, its size and its measure.
 */
void exahype2::fv::tests::ForEachVolumeTest::testVisitsEveryVolumeInLinearisedOrder() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5 * CellWidth);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);
  std::array<double, Volumes * Values>        values{};
  int                                         visits = 0;

  exahype2::fv::forEachVolume<VolumesPerAxis, Values>(
    cellCentre,
    cellSize,
    values.data(),
    [&](double* const value, const auto& x, const auto& h, const double weight, const int index) {
      validateEquals(index, visits);
      validateEquals(value - values.data(), index * Values);
      int remainder = index;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams1(x(axis), remainder % VolumesPerAxis + 0.5, axis);
        validateNumericalEqualsWithParams1(h(axis), 1.0, axis);
        remainder /= VolumesPerAxis;
      }
      validateNumericalEquals(weight, 1.0);
      visits++;
    }
  );
  validateEquals(visits, Volumes);
}

/**
 * Checks that a write through the value pointer reaches the entries of that volume alone.
 */
void exahype2::fv::tests::ForEachVolumeTest::testWritesThroughTheValuePointer() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5 * CellWidth);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(CellWidth);
  std::array<double, Volumes * Values>        values{};

  exahype2::fv::forEachVolume<VolumesPerAxis, Values>(
    cellCentre,
    cellSize,
    values.data(),
    [](
      double* const                 value,
      [[maybe_unused]] const auto&  volumeCentre,
      [[maybe_unused]] const auto&  volumeSize,
      [[maybe_unused]] const double weight,
      const int                     index
    ) {
      value[0] = index;
      value[1] = -index;
    }
  );
  for (int volume = 0; volume < Volumes; volume++) {
    validateNumericalEquals(values[volume * Values], volume);
    validateNumericalEquals(values[volume * Values + 1], -volume);
  }
}
