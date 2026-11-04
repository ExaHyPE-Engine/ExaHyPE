// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/limiting/LimiterStatus.h"

#include "tarch/testing/UnitTest.h"

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

UNIT_TEST("A merge of two limiter statuses returns the more limited one in either order") {
  GIVEN("The limiter statuses regular, halo, limited and troubled") {
    constexpr auto Regular  = exahype2::dg::limiting::LimiterStatus::Regular;
    constexpr auto Halo     = exahype2::dg::limiting::LimiterStatus::Halo;
    constexpr auto Limited  = exahype2::dg::limiting::LimiterStatus::Limited;
    constexpr auto Troubled = exahype2::dg::limiting::LimiterStatus::Troubled;

    WHEN("Pairs of these statuses merge, halo and troubled in both orders") {
      const auto mergeOfRegularAndTroubled = exahype2::dg::limiting::mergeLimiterStatus(Regular, Troubled);
      const auto mergeOfHaloAndLimited     = exahype2::dg::limiting::mergeLimiterStatus(Halo, Limited);
      const auto mergeOfLimitedAndLimited  = exahype2::dg::limiting::mergeLimiterStatus(Limited, Limited);
      const auto mergeOfRegularAndHalo     = exahype2::dg::limiting::mergeLimiterStatus(Regular, Halo);
      const auto mergeOfHaloAndTroubled    = exahype2::dg::limiting::mergeLimiterStatus(Halo, Troubled);
      const auto mergeOfTroubledAndHalo    = exahype2::dg::limiting::mergeLimiterStatus(Troubled, Halo);

      THEN("Every merge returns the more limited status of its pair and both orders give the same status") {
        VALIDATE_EQUALS(static_cast<int>(mergeOfRegularAndTroubled), static_cast<int>(Troubled));
        VALIDATE_EQUALS(static_cast<int>(mergeOfHaloAndLimited), static_cast<int>(Limited));
        VALIDATE_EQUALS(static_cast<int>(mergeOfLimitedAndLimited), static_cast<int>(Limited));
        VALIDATE_EQUALS(static_cast<int>(mergeOfRegularAndHalo), static_cast<int>(Halo));
        VALIDATE_EQUALS(static_cast<int>(mergeOfHaloAndTroubled), static_cast<int>(mergeOfTroubledAndHalo));
      }
    }
  }
}

UNIT_TEST("Only a limited or a troubled cell runs the subcell scheme") {
  GIVEN("The limiter statuses regular, halo, limited and troubled") {
    constexpr auto Regular  = exahype2::dg::limiting::LimiterStatus::Regular;
    constexpr auto Halo     = exahype2::dg::limiting::LimiterStatus::Halo;
    constexpr auto Limited  = exahype2::dg::limiting::LimiterStatus::Limited;
    constexpr auto Troubled = exahype2::dg::limiting::LimiterStatus::Troubled;

    THEN("The limited and the troubled status run the subcell scheme, the regular and the halo status do not") {
      VALIDATE(not exahype2::dg::limiting::runsSubcellScheme(Regular));
      VALIDATE(not exahype2::dg::limiting::runsSubcellScheme(Halo));
      VALIDATE(exahype2::dg::limiting::runsSubcellScheme(Limited));
      VALIDATE(exahype2::dg::limiting::runsSubcellScheme(Troubled));
    }
  }
}

UNIT_TEST("Spreading from a troubled cell grows a limited ring and a halo ring") {
  GIVEN("The limiter statuses regular, halo, limited and troubled") {
    constexpr auto Regular  = exahype2::dg::limiting::LimiterStatus::Regular;
    constexpr auto Halo     = exahype2::dg::limiting::LimiterStatus::Halo;
    constexpr auto Limited  = exahype2::dg::limiting::LimiterStatus::Limited;
    constexpr auto Troubled = exahype2::dg::limiting::LimiterStatus::Troubled;

    WHEN("Every status spreads to a neighbour once") {
      const auto spreadOfTroubled = exahype2::dg::limiting::spreadLimiterStatus(Troubled);
      const auto spreadOfLimited  = exahype2::dg::limiting::spreadLimiterStatus(Limited);
      const auto spreadOfHalo     = exahype2::dg::limiting::spreadLimiterStatus(Halo);
      const auto spreadOfRegular  = exahype2::dg::limiting::spreadLimiterStatus(Regular);

      THEN("Every status lowers by one level and halo and regular both give regular") {
        VALIDATE_EQUALS(static_cast<int>(spreadOfTroubled), static_cast<int>(Limited));
        VALIDATE_EQUALS(static_cast<int>(spreadOfLimited), static_cast<int>(Halo));
        VALIDATE_EQUALS(static_cast<int>(spreadOfHalo), static_cast<int>(Regular));
        VALIDATE_EQUALS(static_cast<int>(spreadOfRegular), static_cast<int>(Regular));
      }
    }

    WHEN("The troubled status spreads over two rings") {
      const auto firstRing  = exahype2::dg::limiting::spreadLimiterStatus(Troubled);
      const auto secondRing = exahype2::dg::limiting::spreadLimiterStatus(firstRing);

      THEN("The first ring runs the subcell scheme and the second ring forms a halo outside it") {
        VALIDATE(exahype2::dg::limiting::runsSubcellScheme(firstRing));
        VALIDATE(not exahype2::dg::limiting::runsSubcellScheme(secondRing));
        VALIDATE_EQUALS(static_cast<int>(secondRing), static_cast<int>(Halo));
      }
    }
  }
}
