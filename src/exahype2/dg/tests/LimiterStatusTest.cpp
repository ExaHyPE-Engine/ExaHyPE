// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimiterStatusTest.h"

#include "exahype2/dg/LimiterStatus.h"

// The pure status functions are compile-time evaluable, so the compiler checks
// these invariants in addition to the runtime cases below.
static_assert(not exahype2::dg::runsSubcellScheme(exahype2::dg::LimiterStatus::Halo));
static_assert(exahype2::dg::runsSubcellScheme(exahype2::dg::LimiterStatus::Limited));
static_assert(
  exahype2::dg::mergeLimiterStatus(exahype2::dg::LimiterStatus::Halo, exahype2::dg::LimiterStatus::Troubled)
  == exahype2::dg::LimiterStatus::Troubled
);
static_assert(
  exahype2::dg::spreadLimiterStatus(exahype2::dg::LimiterStatus::Troubled) == exahype2::dg::LimiterStatus::Limited
);

exahype2::dg::tests::LimiterStatusTest::LimiterStatusTest() :
  tarch::tests::TestCase("exahype2::dg::tests::LimiterStatusTest") {}

void exahype2::dg::tests::LimiterStatusTest::run() {
  testMethod(testMergeKeepsTheMoreLimitedStatus);
  testMethod(testOnlyTroubledAndLimitedRunTheSubcellScheme);
  testMethod(testSpreadingGrowsTwoRings);
  testMethod(testFaceStatusMarkerIsPerSideAndMonotone);
}

void exahype2::dg::tests::LimiterStatusTest::testMergeKeepsTheMoreLimitedStatus() {
  constexpr auto Regular  = exahype2::dg::LimiterStatus::Regular;
  constexpr auto Halo     = exahype2::dg::LimiterStatus::Halo;
  constexpr auto Limited  = exahype2::dg::LimiterStatus::Limited;
  constexpr auto Troubled = exahype2::dg::LimiterStatus::Troubled;

  validateEquals(static_cast<int>(exahype2::dg::mergeLimiterStatus(Regular, Troubled)), static_cast<int>(Troubled));
  validateEquals(static_cast<int>(exahype2::dg::mergeLimiterStatus(Halo, Limited)), static_cast<int>(Limited));
  validateEquals(static_cast<int>(exahype2::dg::mergeLimiterStatus(Limited, Limited)), static_cast<int>(Limited));
  validateEquals(static_cast<int>(exahype2::dg::mergeLimiterStatus(Regular, Halo)), static_cast<int>(Halo));

  validateEquals(
    static_cast<int>(exahype2::dg::mergeLimiterStatus(Halo, Troubled)),
    static_cast<int>(exahype2::dg::mergeLimiterStatus(Troubled, Halo))
  );
}

void exahype2::dg::tests::LimiterStatusTest::testOnlyTroubledAndLimitedRunTheSubcellScheme() {
  validate(not exahype2::dg::runsSubcellScheme(exahype2::dg::LimiterStatus::Regular));
  validate(not exahype2::dg::runsSubcellScheme(exahype2::dg::LimiterStatus::Halo));
  validate(exahype2::dg::runsSubcellScheme(exahype2::dg::LimiterStatus::Limited));
  validate(exahype2::dg::runsSubcellScheme(exahype2::dg::LimiterStatus::Troubled));
}

void exahype2::dg::tests::LimiterStatusTest::testSpreadingGrowsTwoRings() {
  constexpr auto Regular  = exahype2::dg::LimiterStatus::Regular;
  constexpr auto Halo     = exahype2::dg::LimiterStatus::Halo;
  constexpr auto Limited  = exahype2::dg::LimiterStatus::Limited;
  constexpr auto Troubled = exahype2::dg::LimiterStatus::Troubled;

  validateEquals(static_cast<int>(exahype2::dg::spreadLimiterStatus(Troubled)), static_cast<int>(Limited));
  validateEquals(static_cast<int>(exahype2::dg::spreadLimiterStatus(Limited)), static_cast<int>(Halo));
  validateEquals(static_cast<int>(exahype2::dg::spreadLimiterStatus(Halo)), static_cast<int>(Regular));
  validateEquals(static_cast<int>(exahype2::dg::spreadLimiterStatus(Regular)), static_cast<int>(Regular));

  const auto firstRing  = exahype2::dg::spreadLimiterStatus(Troubled);
  const auto secondRing = exahype2::dg::spreadLimiterStatus(firstRing);
  validate(exahype2::dg::runsSubcellScheme(firstRing));
  validate(not exahype2::dg::runsSubcellScheme(secondRing));
  validateEquals(static_cast<int>(secondRing), static_cast<int>(Halo));
}

void exahype2::dg::tests::LimiterStatusTest::testFaceStatusMarkerIsPerSideAndMonotone() {
  constexpr auto Regular  = exahype2::dg::LimiterStatus::Regular;
  constexpr auto Halo     = exahype2::dg::LimiterStatus::Halo;
  constexpr auto Limited  = exahype2::dg::LimiterStatus::Limited;
  constexpr auto Troubled = exahype2::dg::LimiterStatus::Troubled;

  int faceStatus[2] = {static_cast<int>(Regular), static_cast<int>(Regular)};

  exahype2::dg::storeFaceStatus(faceStatus, 0, Limited);
  exahype2::dg::storeFaceStatus(faceStatus, 1, Halo);
  validateEquals(static_cast<int>(exahype2::dg::loadFaceStatus(faceStatus, 0)), static_cast<int>(Limited));
  validateEquals(static_cast<int>(exahype2::dg::loadFaceStatus(faceStatus, 1)), static_cast<int>(Halo));

  exahype2::dg::raiseFaceStatus(faceStatus, 1, Troubled);
  validateEquals(static_cast<int>(exahype2::dg::loadFaceStatus(faceStatus, 1)), static_cast<int>(Troubled));

  exahype2::dg::raiseFaceStatus(faceStatus, 1, Regular);
  validateEquals(static_cast<int>(exahype2::dg::loadFaceStatus(faceStatus, 1)), static_cast<int>(Troubled));

  validateEquals(static_cast<int>(exahype2::dg::loadFaceStatus(faceStatus, 0)), static_cast<int>(Limited));
}
