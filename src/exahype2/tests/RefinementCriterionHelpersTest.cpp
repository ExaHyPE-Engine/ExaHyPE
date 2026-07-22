// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementCriterionHelpersTest.h"

#include "exahype2/RefinementCriterionHelpers.h"
#include "peano4/Globals.h"
#include "peano4/Loop.h"

exahype2::tests::RefinementCriterionHelpersTest::RefinementCriterionHelpersTest() :
  TestCase("exahype2::tests::RefinementCriterionHelpersTest") {}

void exahype2::tests::RefinementCriterionHelpersTest::run() {
  testMethod(testRefinementCommandFromDetail);
  testMethod(testRefineOnDetail);
}

void exahype2::tests::RefinementCriterionHelpersTest::testRefinementCommandFromDetail() {
  constexpr double Eps                 = 1.0e-2;
  constexpr double CoarsenSafetyFactor = 9.0;

  validateTrue(refinementCommandFromDetail(2.0e-2, Eps, CoarsenSafetyFactor) == RefinementCommand::Refine);
  validateTrue(refinementCommandFromDetail(1.0e-2, Eps, CoarsenSafetyFactor) == RefinementCommand::Keep);
  validateTrue(refinementCommandFromDetail(5.0e-3, Eps, CoarsenSafetyFactor) == RefinementCommand::Keep);
  validateTrue(
    refinementCommandFromDetail(Eps / CoarsenSafetyFactor, Eps, CoarsenSafetyFactor) == RefinementCommand::Keep
  );
  validateTrue(refinementCommandFromDetail(1.0e-4, Eps, CoarsenSafetyFactor) == RefinementCommand::Erase);
  validateTrue(refinementCommandFromDetail(0.0, Eps, CoarsenSafetyFactor) == RefinementCommand::Erase);
}

void exahype2::tests::RefinementCriterionHelpersTest::testRefineOnDetail() {
  constexpr int    NumberOfDofsPerAxis = 4;
  constexpr int    Unknowns            = 3;
  constexpr double Eps                 = 1.0e-2;
  constexpr double CoarsenSafetyFactor = 9.0;

  int numberOfVolumes = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    numberOfVolumes *= NumberOfDofsPerAxis;
  }
  std::vector<double> patch(static_cast<std::size_t>(numberOfVolumes) * Unknowns, 0.0);

  const auto density  = [](const double* volume) { return volume[0]; };
  const auto pressure = [](const double* volume) { return volume[2]; };

  // Grid construction hands out no solution, so the criterion keeps the cell. The null
  // pointer carries the patch's type, as a solver's own buffer does.
  const double* const noSolution = nullptr;
  validateTrue(
    refineOnDetail(noSolution, NumberOfDofsPerAxis, Unknowns, Eps, CoarsenSafetyFactor, nullptr, density)
    == RefinementCommand::Keep
  );

  // An affine field is fit exactly, so a smooth patch coarsens.
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * Unknowns;
    patch[index]     = 1.0 + 0.25 * volume(0) - 0.5 * volume(1);
    patch[index + 1] = -3.0;
    patch[index + 2] = 2.0;
  });
  validateTrue(
    refineOnDetail(patch.data(), NumberOfDofsPerAxis, Unknowns, Eps, CoarsenSafetyFactor, nullptr, density, pressure)
    == RefinementCommand::Erase
  );

  // A kink in one monitored unknown refines even when the other stays smooth. The stride and
  // the indicator offset pick the right value out of the AoS layout.
  peano4::dForEach(NumberOfDofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
    const int index  = peano4::dLinearised(volume, NumberOfDofsPerAxis) * Unknowns;
    patch[index + 2] = volume(0) < NumberOfDofsPerAxis / 2 ? 2.0 : 20.0;
  });
  validateTrue(
    refineOnDetail(patch.data(), NumberOfDofsPerAxis, Unknowns, Eps, CoarsenSafetyFactor, nullptr, density)
    == RefinementCommand::Erase
  );
  validateTrue(
    refineOnDetail(patch.data(), NumberOfDofsPerAxis, Unknowns, Eps, CoarsenSafetyFactor, nullptr, density, pressure)
    == RefinementCommand::Refine
  );
}
