// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionRecord.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Request.h"

namespace {
  constexpr std::uint64_t SignBit = std::uint64_t{1} << 63;

  /**
   * Number of leading entries of a packed record. They hold the sizes of the three segments.
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
   * holds the segment sizes. The parameter types follow MPI_User_function.
   */
  void reducePackedRecords(
    void*                          incoming,
    void*                          accumulating,
    int*                           numberOfRecords, // NOLINT(readability-non-const-parameter)
    [[maybe_unused]] MPI_Datatype* datatype
  ) {
    assertion(*numberOfRecords == 1, *numberOfRecords);
    const auto* const source = static_cast<const std::uint64_t*>(incoming);
    auto* const       target = static_cast<std::uint64_t*>(accumulating);
    assertion(source[0] == target[0] and source[1] == target[1] and source[2] == target[2], source[0], target[0]);
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
  assertion(not isPosted());
  _sums.push_back(value);
  return static_cast<int>(_sums.size()) - 1;
}

int
tarch::mpi::ReductionRecord::addMinimum(const std::uint64_t value) {
  assertion(not isPosted());
  _minima.push_back(value);
  return static_cast<int>(_minima.size()) - 1;
}

int
tarch::mpi::ReductionRecord::addMaximum(const std::uint64_t value) {
  assertion(not isPosted());
  _maxima.push_back(value);
  return static_cast<int>(_maxima.size()) - 1;
}

/**
 * Adds every value to the sums and returns the slot of the first. The other values follow it in
 * consecutive slots.
 */
int
tarch::mpi::ReductionRecord::addSums(const std::span<const std::uint64_t> values) {
  assertion(not values.empty());
  assertion(not isPosted());
  _sums.insert(_sums.end(), values.begin(), values.end());
  return static_cast<int>(_sums.size() - values.size());
}

/**
 * Adds every value to the minima and returns the slot of the first. The other values follow it in
 * consecutive slots.
 */
int
tarch::mpi::ReductionRecord::addMinima(const std::span<const std::uint64_t> values) {
  assertion(not values.empty());
  assertion(not isPosted());
  _minima.insert(_minima.end(), values.begin(), values.end());
  return static_cast<int>(_minima.size() - values.size());
}

/**
 * Adds every value to the maxima and returns the slot of the first. The other values follow it in
 * consecutive slots.
 */
int
tarch::mpi::ReductionRecord::addMaxima(const std::span<const std::uint64_t> values) {
  assertion(not values.empty());
  assertion(not isPosted());
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

tarch::mpi::ReductionRecord::~ReductionRecord() { assertion(not isPosted()); }

/**
 * Replaces every value by its sum, minimum or maximum over the ranks of communicator. Every rank of
 * communicator calls this function with a record of the same layout.
 */
void
tarch::mpi::ReductionRecord::allreduce(MPI_Comm communicator) {
  post(communicator);
  wait();
}

/**
 * Starts the reduction of allreduce() and returns at once. Every rank of communicator posts a record
 * of the same layout. Records reduce in the order in which the ranks post them. The values change
 * only in wait().
 */
void
tarch::mpi::ReductionRecord::post(MPI_Comm communicator) {
  assertion(not isPosted());
  _packed.clear();
  _packed.reserve(HeaderSize + _sums.size() + _minima.size() + _maxima.size());
  _packed.push_back(_sums.size());
  _packed.push_back(_minima.size());
  _packed.push_back(_maxima.size());
  _packed.insert(_packed.end(), _sums.begin(), _sums.end());
  _packed.insert(_packed.end(), _minima.begin(), _minima.end());
  _packed.insert(_packed.end(), _maxima.begin(), _maxima.end());
  assertion(_packed.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()), _packed.size());

  // One element of the record type spans the whole record. A pipelined reduction never splits an
  // element. The operation sees the header in every call.
  tarch::mpi::createContiguousType(static_cast<int>(_packed.size()), MPI_UINT64_T, &_recordType);
  tarch::mpi::commitType(&_recordType);
  tarch::mpi::createOperation(reducePackedRecords, true, &_operation);
  tarch::mpi::iallreduce(MPI_IN_PLACE, _packed.data(), 1, _recordType, _operation, communicator, &_request);
}

/**
 * Completes the reduction that post() started and replaces every value by its reduced value.
 */
void
tarch::mpi::ReductionRecord::wait() {
  assertion(isPosted());
  tarch::mpi::wait(&_request);
  tarch::mpi::freeOperation(&_operation);
  tarch::mpi::freeType(&_recordType);

  const auto firstMinimum = _packed.begin() + HeaderSize + static_cast<std::ptrdiff_t>(_sums.size());
  const auto firstMaximum = firstMinimum + static_cast<std::ptrdiff_t>(_minima.size());
  std::copy(_packed.begin() + HeaderSize, firstMinimum, _sums.begin());
  std::copy(firstMinimum, firstMaximum, _minima.begin());
  std::copy(firstMaximum, _packed.end(), _maxima.begin());
}

bool
tarch::mpi::ReductionRecord::isPosted() const {
  return _recordType != MPI_DATATYPE_NULL;
}

/**
 * Returns whether other holds as many sums, minima and maxima as this record. Records of the same
 * layout assign the same slots.
 */
bool
tarch::mpi::ReductionRecord::hasLayoutOf(const ReductionRecord& other) const {
  return _sums.size() == other._sums.size() and _minima.size() == other._minima.size()
     and _maxima.size() == other._maxima.size();
}

std::uint64_t
tarch::mpi::ReductionRecord::getSum(const int slot) const {
  assertion(not isPosted());
  assertion(slot >= 0 and slot < static_cast<int>(_sums.size()), slot, _sums.size());
  return _sums[static_cast<std::size_t>(slot)];
}

std::uint64_t
tarch::mpi::ReductionRecord::getMinimum(const int slot) const {
  assertion(not isPosted());
  assertion(slot >= 0 and slot < static_cast<int>(_minima.size()), slot, _minima.size());
  return _minima[static_cast<std::size_t>(slot)];
}

std::uint64_t
tarch::mpi::ReductionRecord::getMaximum(const int slot) const {
  assertion(not isPosted());
  assertion(slot >= 0 and slot < static_cast<int>(_maxima.size()), slot, _maxima.size());
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
