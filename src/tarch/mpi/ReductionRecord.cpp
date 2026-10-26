// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionRecord.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"

namespace {
  constexpr std::uint64_t SignBit = std::uint64_t{1} << 63;

  /**
   * Number of leading entries of a packed record. They store the sizes of the three segments.
   */
  constexpr int HeaderSize = 3;

  /**
   * Returns a key whose unsigned order matches the order of value.
   */
  std::uint64_t toOrderedKey(const double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    return (bits & SignBit) != 0 ? ~bits : (bits | SignBit);
  }

  double fromOrderedKey(const std::uint64_t key) {
    return std::bit_cast<double>((key & SignBit) != 0 ? (key & ~SignBit) : ~key);
  }

  /**
   * Sums, minimises and maximises the three segments of one packed record. The header of the record
   * stores the segment sizes. The parameter types follow MPI_User_function.
   */
  void reducePackedRecords(
    void*                          incoming,
    void*                          accumulating,
    int*                           numberOfRecords, // NOLINT(readability-non-const-parameter)
    [[maybe_unused]] MPI_Datatype* datatype
  ) {
    ASSERTION(*numberOfRecords == 1, *numberOfRecords);
    const auto* const source = static_cast<const std::uint64_t*>(incoming);
    auto* const       target = static_cast<std::uint64_t*>(accumulating);
    ASSERTION(source[0] == target[0] and source[1] == target[1] and source[2] == target[2], source[0], target[0]);
    const std::uint64_t firstMinimum = HeaderSize + target[0];
    const std::uint64_t firstMaximum = firstMinimum + target[1];
    const std::uint64_t end          = firstMaximum + target[2];
    for (std::uint64_t entry = HeaderSize; entry < firstMinimum; entry++) {
      target[entry] += source[entry];
    }
    for (std::uint64_t entry = firstMinimum; entry < firstMaximum; entry++) {
      target[entry] = std::min(target[entry], source[entry]);
    }
    for (std::uint64_t entry = firstMaximum; entry < end; entry++) {
      target[entry] = std::max(target[entry], source[entry]);
    }
  }
} // namespace

int
tarch::mpi::ReductionRecord::addSum(const std::uint64_t value) {
  _sums.push_back(value);
  return static_cast<int>(_sums.size()) - 1;
}

int
tarch::mpi::ReductionRecord::addMinimum(const std::uint64_t value) {
  _minima.push_back(value);
  return static_cast<int>(_minima.size()) - 1;
}

int
tarch::mpi::ReductionRecord::addMaximum(const std::uint64_t value) {
  _maxima.push_back(value);
  return static_cast<int>(_maxima.size()) - 1;
}

/**
 * Adds every value to the sums and returns the slot of the first. The other values follow it in
 * consecutive slots.
 */
int
tarch::mpi::ReductionRecord::addSums(const std::span<const std::uint64_t> values) {
  ASSERTION(not values.empty());
  _sums.insert(_sums.end(), values.begin(), values.end());
  return static_cast<int>(_sums.size() - values.size());
}

/**
 * Adds every value to the minima and returns the slot of the first. The other values follow it in
 * consecutive slots.
 */
int
tarch::mpi::ReductionRecord::addMinima(const std::span<const std::uint64_t> values) {
  ASSERTION(not values.empty());
  _minima.insert(_minima.end(), values.begin(), values.end());
  return static_cast<int>(_minima.size() - values.size());
}

/**
 * Adds every value to the maxima and returns the slot of the first. The other values follow it in
 * consecutive slots.
 */
int
tarch::mpi::ReductionRecord::addMaxima(const std::span<const std::uint64_t> values) {
  ASSERTION(not values.empty());
  _maxima.insert(_maxima.end(), values.begin(), values.end());
  return static_cast<int>(_maxima.size() - values.size());
}

int
tarch::mpi::ReductionRecord::addMinimumOfDouble(const double value) {
  return addMinimum(toOrderedKey(value));
}

int
tarch::mpi::ReductionRecord::addMaximumOfDouble(const double value) {
  return addMaximum(toOrderedKey(value));
}

/**
 * Replaces every value by its sum, minimum or maximum over the ranks of communicator. Every rank of
 * communicator calls this function with a record of the same layout.
 */
void
tarch::mpi::ReductionRecord::allreduce(MPI_Comm communicator) {
  std::vector<std::uint64_t> packed;
  packed.reserve(HeaderSize + _sums.size() + _minima.size() + _maxima.size());
  packed.push_back(_sums.size());
  packed.push_back(_minima.size());
  packed.push_back(_maxima.size());
  packed.insert(packed.end(), _sums.begin(), _sums.end());
  packed.insert(packed.end(), _minima.begin(), _minima.end());
  packed.insert(packed.end(), _maxima.begin(), _maxima.end());
  ASSERTION(packed.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()), packed.size());

  // One element of the record type spans the whole record. The operation then sees the header in
  // every call of a pipelined reduction.
  MPI_Datatype recordType = MPI_DATATYPE_NULL;
  MPI_Op       operation  = MPI_OP_NULL;
  tarch::mpi::createContiguousType(static_cast<int>(packed.size()), MPI_UINT64_T, &recordType);
  tarch::mpi::commitType(&recordType);
  tarch::mpi::createOperation(reducePackedRecords, true, &operation);
  tarch::mpi::allreduce(MPI_IN_PLACE, packed.data(), 1, recordType, operation, communicator);
  tarch::mpi::freeOperation(&operation);
  tarch::mpi::freeType(&recordType);

  const auto firstMinimum = packed.begin() + HeaderSize + static_cast<std::ptrdiff_t>(_sums.size());
  const auto firstMaximum = firstMinimum + static_cast<std::ptrdiff_t>(_minima.size());
  std::copy(packed.begin() + HeaderSize, firstMinimum, _sums.begin());
  std::copy(firstMinimum, firstMaximum, _minima.begin());
  std::copy(firstMaximum, packed.end(), _maxima.begin());
}

std::uint64_t
tarch::mpi::ReductionRecord::getSum(const int slot) const {
  ASSERTION(slot >= 0 and slot < static_cast<int>(_sums.size()), slot, _sums.size());
  return _sums[static_cast<std::size_t>(slot)];
}

std::uint64_t
tarch::mpi::ReductionRecord::getMinimum(const int slot) const {
  ASSERTION(slot >= 0 and slot < static_cast<int>(_minima.size()), slot, _minima.size());
  return _minima[static_cast<std::size_t>(slot)];
}

std::uint64_t
tarch::mpi::ReductionRecord::getMaximum(const int slot) const {
  ASSERTION(slot >= 0 and slot < static_cast<int>(_maxima.size()), slot, _maxima.size());
  return _maxima[static_cast<std::size_t>(slot)];
}

double
tarch::mpi::ReductionRecord::getMinimumOfDouble(const int slot) const {
  return fromOrderedKey(getMinimum(slot));
}

double
tarch::mpi::ReductionRecord::getMaximumOfDouble(const int slot) const {
  return fromOrderedKey(getMaximum(slot));
}
