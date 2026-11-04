// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimiterStatusTest.h"

#include "exahype2/dg/limiting/LimiterStatus.h"

static_assert(not exahype2::dg::limiting::runsSubcellScheme(exahype2::dg::limiting::LimiterStatus::Halo));
static_assert(exahype2::dg::limiting::runsSubcellScheme(exahype2::dg::limiting::LimiterStatus::Limited));
static_assert(
  exahype2::dg::limiting::mergeLimiterStatus(
    exahype2::dg::limiting::LimiterStatus::Halo,
    exahype2::dg::limiting::LimiterStatus::Troubled
  )
  == exahype2::dg::limiting::LimiterStatus::Troubled
);
static_assert(
  exahype2::dg::limiting::spreadLimiterStatus(exahype2::dg::limiting::LimiterStatus::Troubled)
  == exahype2::dg::limiting::LimiterStatus::Limited
);
static_assert(exahype2::dg::limiting::NumberOfSpreadRings == 2);
static_assert(exahype2::dg::limiting::NumberOfStatusEntries == 4);

exahype2::dg::limiting::tests::LimiterStatusTest::LimiterStatusTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::LimiterStatusTest") {}

void
exahype2::dg::limiting::tests::LimiterStatusTest::run() {
  testMethod(testMergeKeepsTheMoreLimitedStatus);
  testMethod(testOnlyTroubledAndLimitedRunTheSubcellScheme);
  testMethod(testSpreadingGrowsTwoRings);
}

void
exahype2::dg::limiting::tests::LimiterStatusTest::testMergeKeepsTheMoreLimitedStatus() {
  constexpr auto Regular  = exahype2::dg::limiting::LimiterStatus::Regular;
  constexpr auto Halo     = exahype2::dg::limiting::LimiterStatus::Halo;
  constexpr auto Limited  = exahype2::dg::limiting::LimiterStatus::Limited;
  constexpr auto Troubled = exahype2::dg::limiting::LimiterStatus::Troubled;

  validateEquals(
    static_cast<int>(exahype2::dg::limiting::mergeLimiterStatus(Regular, Troubled)),
    static_cast<int>(Troubled)
  );
  validateEquals(
    static_cast<int>(exahype2::dg::limiting::mergeLimiterStatus(Halo, Limited)),
    static_cast<int>(Limited)
  );
  validateEquals(
    static_cast<int>(exahype2::dg::limiting::mergeLimiterStatus(Limited, Limited)),
    static_cast<int>(Limited)
  );
  validateEquals(static_cast<int>(exahype2::dg::limiting::mergeLimiterStatus(Regular, Halo)), static_cast<int>(Halo));

  validateEquals(
    static_cast<int>(exahype2::dg::limiting::mergeLimiterStatus(Halo, Troubled)),
    static_cast<int>(exahype2::dg::limiting::mergeLimiterStatus(Troubled, Halo))
  );
}

void
exahype2::dg::limiting::tests::LimiterStatusTest::testOnlyTroubledAndLimitedRunTheSubcellScheme() {
  validate(not exahype2::dg::limiting::runsSubcellScheme(exahype2::dg::limiting::LimiterStatus::Regular));
  validate(not exahype2::dg::limiting::runsSubcellScheme(exahype2::dg::limiting::LimiterStatus::Halo));
  validate(exahype2::dg::limiting::runsSubcellScheme(exahype2::dg::limiting::LimiterStatus::Limited));
  validate(exahype2::dg::limiting::runsSubcellScheme(exahype2::dg::limiting::LimiterStatus::Troubled));
}

void
exahype2::dg::limiting::tests::LimiterStatusTest::testSpreadingGrowsTwoRings() {
  constexpr auto Regular  = exahype2::dg::limiting::LimiterStatus::Regular;
  constexpr auto Halo     = exahype2::dg::limiting::LimiterStatus::Halo;
  constexpr auto Limited  = exahype2::dg::limiting::LimiterStatus::Limited;
  constexpr auto Troubled = exahype2::dg::limiting::LimiterStatus::Troubled;

  validateEquals(static_cast<int>(exahype2::dg::limiting::spreadLimiterStatus(Troubled)), static_cast<int>(Limited));
  validateEquals(static_cast<int>(exahype2::dg::limiting::spreadLimiterStatus(Limited)), static_cast<int>(Halo));
  validateEquals(static_cast<int>(exahype2::dg::limiting::spreadLimiterStatus(Halo)), static_cast<int>(Regular));
  validateEquals(static_cast<int>(exahype2::dg::limiting::spreadLimiterStatus(Regular)), static_cast<int>(Regular));

  const auto firstRing  = exahype2::dg::limiting::spreadLimiterStatus(Troubled);
  const auto secondRing = exahype2::dg::limiting::spreadLimiterStatus(firstRing);
  validate(exahype2::dg::limiting::runsSubcellScheme(firstRing));
  validate(not exahype2::dg::limiting::runsSubcellScheme(secondRing));
  validateEquals(static_cast<int>(secondRing), static_cast<int>(Halo));
}
