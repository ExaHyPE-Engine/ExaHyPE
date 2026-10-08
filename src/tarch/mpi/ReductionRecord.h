// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

/**
 * Collects values that one allreduce sums, minimises or maximises over all ranks. Every rank adds
 * its values in the same order. Each add function returns the slot of its value. After allreduce(),
 * or after post() and wait(), the getter of that slot returns the reduced value. MPI writes into
 * the record between post() and wait(). The record therefore stays at its address.
 */
class tarch::mpi::ReductionRecord final {
public:
  ReductionRecord()                                  = default;
  ReductionRecord(const ReductionRecord&)            = delete;
  ReductionRecord(ReductionRecord&&)                 = delete;
  ReductionRecord& operator=(const ReductionRecord&) = delete;
  ReductionRecord& operator=(ReductionRecord&&)      = delete;
  ~ReductionRecord();

  [[nodiscard]] int addSum(std::uint64_t value);
  [[nodiscard]] int addMinimum(std::uint64_t value);
  [[nodiscard]] int addMaximum(std::uint64_t value);
  [[nodiscard]] int addSums(std::span<const std::uint64_t> values);
  [[nodiscard]] int addMinima(std::span<const std::uint64_t> values);
  [[nodiscard]] int addMaxima(std::span<const std::uint64_t> values);
  [[nodiscard]] int addMinimumOfDouble(double value);
  [[nodiscard]] int addMaximumOfDouble(double value);

  void allreduce(MPI_Comm communicator);
  void post(MPI_Comm communicator);
  void wait();
  [[nodiscard]] bool isPosted() const;
  [[nodiscard]] bool hasLayoutOf(const ReductionRecord& other) const;

  [[nodiscard]] std::uint64_t getSum(int slot) const;
  [[nodiscard]] std::uint64_t getMinimum(int slot) const;
  [[nodiscard]] std::uint64_t getMaximum(int slot) const;
  [[nodiscard]] double getMinimumOfDouble(int slot) const;
  [[nodiscard]] double getMaximumOfDouble(int slot) const;

private:
  std::vector<std::uint64_t> _sums{};
  std::vector<std::uint64_t> _minima{};
  std::vector<std::uint64_t> _maxima{};
  std::vector<std::uint64_t> _packed{};
  MPI_Datatype               _recordType{MPI_DATATYPE_NULL};
  MPI_Op                     _operation{MPI_OP_NULL};
  MPI_Request                _request{MPI_REQUEST_NULL};
};
