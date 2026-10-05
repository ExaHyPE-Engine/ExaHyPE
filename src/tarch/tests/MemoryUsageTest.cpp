// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryUsageTest.h"

#include "tarch/MemoryUsage.h"

#include <sys/mman.h>

tarch::tests::MemoryUsageTest::MemoryUsageTest() :
  tarch::tests::TestCase("tarch::tests::MemoryUsageTest") {}

void
tarch::tests::MemoryUsageTest::run() {
  testMethod(testAllocatedBytesCountTheCapacity);
  testMethod(testFormatPicksTheLargestReachedUnit);
  testMethod(testProcessAndNodeMemoryAreConsistent);
}

void
tarch::tests::MemoryUsageTest::testAllocatedBytesCountTheCapacity() {
  std::vector<double> values;
  validateEquals(tarch::getAllocatedBytesOf(values), std::size_t{0});
  values.reserve(10);
  values.push_back(1.0);
  validateEquals(tarch::getAllocatedBytesOf(values), values.capacity() * sizeof(double));
  validate(tarch::getAllocatedBytesOf(values) >= 10 * sizeof(double));
}

/**
 * Every size renders in the largest unit it reaches, and a size below one MiB renders in KiB.
 */
void
tarch::tests::MemoryUsageTest::testFormatPicksTheLargestReachedUnit() {
  static_assert(tarch::chooseMemoryUsageFormat(0) == tarch::MemoryUsageFormat::KiB);
  validate(tarch::chooseMemoryUsageFormat(tarch::MiB - 1) == tarch::MemoryUsageFormat::KiB);
  validate(tarch::chooseMemoryUsageFormat(tarch::MiB) == tarch::MemoryUsageFormat::MiB);
  validate(tarch::chooseMemoryUsageFormat(tarch::GiB - 1) == tarch::MemoryUsageFormat::MiB);
  validate(tarch::chooseMemoryUsageFormat(tarch::GiB) == tarch::MemoryUsageFormat::GiB);
  validateEquals(tarch::toString(tarch::MemoryUsageFormat::MiB), std::string("MiB"));
  validateNumericalEquals(tarch::convertMemorySize(3 * tarch::MiB, tarch::MemoryUsageFormat::MiB), 3.0);
  validateNumericalEquals(tarch::convertMemorySize(tarch::GiB, tarch::MemoryUsageFormat::KiB), 1048576.0);
}

/**
 * The resident set of the process grows with fresh pages it writes, and the available memory of
 * the node never exceeds its total memory.
 */
void
tarch::tests::MemoryUsageTest::testProcessAndNodeMemoryAreConsistent() {
  const auto before = tarch::getMemoryUsageInBytes();
  validate(before > 0);

  // The heap may serve a vector from freed pages that stay resident, so the pages come from a
  // mapping of their own.
  constexpr std::size_t WrittenBytes = 64 * tarch::MiB;
  void* const mapping = mmap(nullptr, WrittenBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  validate(mapping != MAP_FAILED);
  std::memset(mapping, 1, WrittenBytes);
  const auto after = tarch::getMemoryUsageInBytes();
  munmap(mapping, WrittenBytes);
  validateWithParams2(after >= before + WrittenBytes / 2, before, after);

  const auto total     = tarch::getTotalMemoryInBytes();
  const auto available = tarch::getAvailableMemoryInBytes();
  validate(total > 0);
  validateWithParams2(available > 0 and available <= total, available, total);
}
