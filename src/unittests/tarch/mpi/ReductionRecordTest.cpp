// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/ReductionRecord.h"

#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Returns the double that rank adds. Even ranks add negative and odd ranks add positive values.
   */
  double getDoubleOfRank(const int rank) { return rank % 2 == 0 ? -1.5 - rank : 0.25 * rank; }
} // namespace

UNIT_TEST("Every slot returns the sum, the minimum or the maximum of the values that the ranks add to it") {
  GIVEN("The rank, the number of ranks and the smallest and the largest double that a rank adds") {
    const auto& rank             = tarch::mpi::Rank::getInstance();
    const auto  ranks            = static_cast<std::uint64_t>(rank.getNumberOfRanks());
    const auto  thisRank         = static_cast<std::uint64_t>(rank.getRank());
    const auto  doubleOfThisRank = getDoubleOfRank(rank.getRank());
    double      expectedMinimum  = getDoubleOfRank(0);
    double      expectedMaximum  = getDoubleOfRank(0);
    for (int other = 1; other < rank.getNumberOfRanks(); other++) {
      expectedMinimum = std::min(expectedMinimum, getDoubleOfRank(other));
      expectedMaximum = std::max(expectedMaximum, getDoubleOfRank(other));
    }

    WHEN("The ranks reduce a record of single values and a pair that every rank derives from its rank") {
      tarch::mpi::ReductionRecord        record;
      const int                          sumSlot           = record.addSum(thisRank + 1);
      const int                          minimumSlot       = record.addMinimum(thisRank + 5);
      const int                          maximumSlot       = record.addMaximum(thisRank + 5);
      const int                          secondSumSlot     = record.addSum(2);
      const int                          minimumDoubleSlot = record.addMinimumOfDouble(doubleOfThisRank);
      const int                          maximumDoubleSlot = record.addMaximumOfDouble(doubleOfThisRank);
      const std::array<std::uint64_t, 2> pair{thisRank, 7};
      const int                          firstSumOfPair     = record.addSums(pair);
      const int                          firstMinimumOfPair = record.addMinima(pair);
      const int                          firstMaximumOfPair = record.addMaxima(pair);
      record.allreduce(rank.getCommunicator());

      THEN("The sums take consecutive slots and add the values of every rank") {
        VALIDATE_EQUALS(sumSlot, 0);
        VALIDATE_EQUALS(secondSumSlot, 1);
        VALIDATE_EQUALS(firstSumOfPair, 2);
        VALIDATE_EQUALS(record.getSum(sumSlot), ranks * (ranks + 1) / 2);
        VALIDATE_EQUALS(record.getSum(secondSumSlot), 2 * ranks);
        VALIDATE_EQUALS(record.getSum(firstSumOfPair), ranks * (ranks - 1) / 2);
        VALIDATE_EQUALS(record.getSum(firstSumOfPair + 1), 7 * ranks);
      }

      THEN("The minima and the maxima take the smallest and the largest value over the ranks") {
        VALIDATE_EQUALS(record.getMinimum(minimumSlot), std::uint64_t{5});
        VALIDATE_EQUALS(record.getMaximum(maximumSlot), ranks + 4);
        VALIDATE_EQUALS(record.getMinimumOfDouble(minimumDoubleSlot), expectedMinimum);
        VALIDATE_EQUALS(record.getMaximumOfDouble(maximumDoubleSlot), expectedMaximum);
        VALIDATE_EQUALS(record.getMinimum(firstMinimumOfPair), std::uint64_t{0});
        VALIDATE_EQUALS(record.getMinimum(firstMinimumOfPair + 1), std::uint64_t{7});
        VALIDATE_EQUALS(record.getMaximum(firstMaximumOfPair), ranks - 1);
        VALIDATE_EQUALS(record.getMaximum(firstMaximumOfPair + 1), std::uint64_t{7});
      }
    }
  }
}

UNIT_TEST("A double that every rank adds keeps its bits through the reduction, signed zeros and infinities included") {
  GIVEN("Eight doubles from minus infinity over both zeros and the smallest subnormal number to infinity") {
    constexpr std::array<double, 8> Values{
      -std::numeric_limits<double>::infinity(),
      -3.5,
      -0.0,
      0.0,
      std::numeric_limits<double>::denorm_min(),
      2.0,
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::infinity()
    };

    WHEN("The ranks reduce a record that adds every value as a minimum and as a maximum") {
      tarch::mpi::ReductionRecord    record;
      std::array<int, Values.size()> minimumSlots{};
      std::array<int, Values.size()> maximumSlots{};
      for (std::size_t entry = 0; entry < Values.size(); entry++) {
        minimumSlots[entry] = record.addMinimumOfDouble(Values[entry]);
        maximumSlots[entry] = record.addMaximumOfDouble(Values[entry]);
      }
      record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());

      THEN("Every minimum and every maximum returns the bits of its value") {
        for (std::size_t entry = 0; entry < Values.size(); entry++) {
          const auto expectedBits = std::bit_cast<std::uint64_t>(Values[entry]);
          VALIDATE_EQUALS(
            std::bit_cast<std::uint64_t>(record.getMinimumOfDouble(minimumSlots[entry])),
            expectedBits,
            entry
          );
          VALIDATE_EQUALS(
            std::bit_cast<std::uint64_t>(record.getMaximumOfDouble(maximumSlots[entry])),
            expectedBits,
            entry
          );
        }
      }
    }
  }
}
