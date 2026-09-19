// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ForEachSubcellTest.h"

#include "exahype2/dg/limiting/ForEachSubcell.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Constants.h"
#include "tarch/la/Vector.h"

exahype2::dg::limiting::tests::ForEachSubcellTest::ForEachSubcellTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::ForEachSubcellTest") {}

void exahype2::dg::limiting::tests::ForEachSubcellTest::run() { testMethod(testVisitsEverySubcellAtItsPatchCentre); }

/**
 * Checks that the loop visits the subcells in linearised order at the centres of
 * exahype2::fv::getVolumeSizeAndCentre, bit for bit, with the subcell measure as weight.
 */
void exahype2::dg::limiting::tests::ForEachSubcellTest::testVisitsEverySubcellAtItsPatchCentre() {
  constexpr int SubcellsPerAxis  = 3;
  constexpr int Values           = 2;
  constexpr int NumberOfSubcells = peano4::power(SubcellsPerAxis, DIMENSIONS);

  double                                      values[NumberOfSubcells * Values]{};
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.3);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.7);

  double expectedWeight = 1.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    expectedWeight *= cellSize(axis) / SubcellsPerAxis;
  }

  int visitedSubcells = 0;
  exahype2::dg::limiting::forEachSubcell<SubcellsPerAxis, Values>(
    cellCentre,
    cellSize,
    values,
    [&](
      const double* const                                           value,
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      const double                                                  weight,
      const int                                                     index
    ) {
      validateEquals(index, visitedSubcells);
      validateTrue(value == values + index * Values);

      tarch::la::Vector<DIMENSIONS, int> subcell;
      int                                remainder = index;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        subcell(axis) = remainder % SubcellsPerAxis;
        remainder /= SubcellsPerAxis;
      }
      const auto expectedCentre =
        exahype2::fv::getVolumeSizeAndCentre<DIMENSIONS, SubcellsPerAxis>(cellCentre, cellSize, subcell).volumeCentre;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEquals(std::bit_cast<std::uint64_t>(x(axis)), std::bit_cast<std::uint64_t>(expectedCentre(axis)));
      }
      validateEquals(std::bit_cast<std::uint64_t>(weight), std::bit_cast<std::uint64_t>(expectedWeight));
      visitedSubcells++;
    }
  );
  validateEquals(visitedSubcells, NumberOfSubcells);
}
