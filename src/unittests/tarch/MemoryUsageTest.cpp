// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/MemoryUsage.h"

#include "tarch/testing/UnitTest.h"

#include <sys/mman.h>

UNIT_TEST("The allocated bytes of a vector count its whole capacity, the unused capacity included") {
  GIVEN("An empty vector of doubles") {
    std::vector<double> values;

    WHEN("The vector reserves ten entries and receives one value") {
      const std::size_t bytesBeforeTheReservation = tarch::getAllocatedBytesOf(values);
      values.reserve(10);
      values.push_back(1.0);
      const std::size_t bytesAfterTheReservation = tarch::getAllocatedBytesOf(values);

      THEN("The vector counts zero bytes before the reservation and its capacity of at least ten entries afterwards") {
        VALIDATE_EQUALS(bytesBeforeTheReservation, std::size_t{0});
        VALIDATE_EQUALS(bytesAfterTheReservation, values.capacity() * sizeof(double));
        VALIDATE(bytesAfterTheReservation >= 10 * sizeof(double));
      }
    }
  }
}

UNIT_TEST("Every size renders in the largest unit it reaches and a size below one MiB renders in KiB") {
  GIVEN("The sizes zero, one byte below one MiB, one MiB, one byte below one GiB and one GiB") {
    constexpr unsigned long long NoBytes     = 0;
    constexpr unsigned long long BelowOneMiB = tarch::MiB - 1;
    constexpr unsigned long long OneMiB      = tarch::MiB;
    constexpr unsigned long long BelowOneGiB = tarch::GiB - 1;
    constexpr unsigned long long OneGiB      = tarch::GiB;

    WHEN("The test chooses the format of every size") {
      constexpr tarch::MemoryUsageFormat FormatOfNoBytes   = tarch::chooseMemoryUsageFormat(NoBytes);
      const tarch::MemoryUsageFormat     formatBelowOneMiB = tarch::chooseMemoryUsageFormat(BelowOneMiB);
      const tarch::MemoryUsageFormat     formatOfOneMiB    = tarch::chooseMemoryUsageFormat(OneMiB);
      const tarch::MemoryUsageFormat     formatBelowOneGiB = tarch::chooseMemoryUsageFormat(BelowOneGiB);
      const tarch::MemoryUsageFormat     formatOfOneGiB    = tarch::chooseMemoryUsageFormat(OneGiB);

      THEN("The sizes below one MiB render in KiB, the sizes below one GiB in MiB and one GiB in GiB") {
        static_assert(FormatOfNoBytes == tarch::MemoryUsageFormat::KiB);
        VALIDATE(formatBelowOneMiB == tarch::MemoryUsageFormat::KiB);
        VALIDATE(formatOfOneMiB == tarch::MemoryUsageFormat::MiB);
        VALIDATE(formatBelowOneGiB == tarch::MemoryUsageFormat::MiB);
        VALIDATE(formatOfOneGiB == tarch::MemoryUsageFormat::GiB);
      }
    }
  }
}

UNIT_TEST("A size converts into the unit of a format and the format returns the name of its unit") {
  GIVEN("The sizes three MiB and one GiB") {
    constexpr unsigned long long ThreeMiB = 3 * tarch::MiB;
    constexpr unsigned long long OneGiB   = tarch::GiB;

    WHEN("Three MiB convert into MiB, one GiB converts into KiB and the format MiB returns its name") {
      const double      threeMiBInMiB = tarch::convertMemorySize(ThreeMiB, tarch::MemoryUsageFormat::MiB);
      const double      oneGiBInKiB   = tarch::convertMemorySize(OneGiB, tarch::MemoryUsageFormat::KiB);
      const std::string nameOfMiB     = tarch::toString(tarch::MemoryUsageFormat::MiB);

      THEN("The converted sizes are three and 1048576 and the name is MiB") {
        VALIDATE_EQUALS(nameOfMiB, std::string("MiB"));
        VALIDATE_NUMERICAL_EQUALS(threeMiBInMiB, 3.0);
        VALIDATE_NUMERICAL_EQUALS(oneGiBInKiB, 1048576.0);
      }
    }
  }
}

UNIT_TEST("The resident set of the process grows with fresh pages it writes") {
  GIVEN("The size of 64 MiB") {
    constexpr std::size_t WrittenBytes = 64 * tarch::MiB;

    WHEN("The process writes every page of a private anonymous mapping of this size") {
      // The heap may serve a vector from freed pages that stay resident. The pages therefore come
      // from a mapping of their own.
      const auto    before   = tarch::getMemoryUsageInBytes();
      void* const   mapping  = mmap(nullptr, WrittenBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      const bool    isMapped = mapping != MAP_FAILED;
      std::uint64_t after    = 0;
      if (isMapped) {
        std::memset(mapping, 1, WrittenBytes);
        after = tarch::getMemoryUsageInBytes();
        munmap(mapping, WrittenBytes);
      }

      THEN("The resident set is positive and grows by at least half of the written pages") {
        VALIDATE(before > 0);
        VALIDATE(isMapped);
        VALIDATE(after >= before + WrittenBytes / 2, before, after);
      }
    }
  }
}

UNIT_TEST("The available memory of the node never exceeds its total memory") {
  WHEN("The test reads the total and the available memory of the node") {
    const auto total     = tarch::getTotalMemoryInBytes();
    const auto available = tarch::getAvailableMemoryInBytes();

    THEN("The total and the available memory are positive and the available memory is at most the total memory") {
      VALIDATE(total > 0);
      VALIDATE(available > 0 and available <= total, available, total);
    }
  }
}
