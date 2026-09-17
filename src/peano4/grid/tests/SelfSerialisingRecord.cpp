// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SelfSerialisingRecord.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Request.h"

peano4::grid::tests::SelfSerialisingRecord::SelfSerialisingRecord(
  const std::size_t numberOfValues,
  const DoFType     value
) {
  _values.reserve(numberOfValues);
  for (std::size_t index = 0; index < numberOfValues; index++) {
    _values.push_back(new DoFType(value));
  }
}

peano4::grid::tests::SelfSerialisingRecord::SelfSerialisingRecord(const SelfSerialisingRecord& other) {
  _values.reserve(other._values.size());
  for (const DoFType* const value : other._values) {
    _values.push_back(new DoFType(*value));
  }
}

peano4::grid::tests::SelfSerialisingRecord::SelfSerialisingRecord(SelfSerialisingRecord&& other) noexcept :
  _values(std::move(other._values)) {
  other._values.clear();
}

peano4::grid::tests::SelfSerialisingRecord& peano4::grid::tests::SelfSerialisingRecord::operator=(
  SelfSerialisingRecord&& other
) noexcept {
  if (this != &other) {
    clear();
    _values = std::move(other._values);
    other._values.clear();
  }
  return *this;
}

peano4::grid::tests::SelfSerialisingRecord::~SelfSerialisingRecord() noexcept { clear(); }

peano4::grid::tests::SelfSerialisingRecord::DoFType peano4::grid::tests::SelfSerialisingRecord::at(
  const std::size_t index
) const {
  assertion(index < _values.size(), index, _values.size());
  return *_values[index];
}

std::size_t peano4::grid::tests::SelfSerialisingRecord::size() const { return _values.size(); }

void peano4::grid::tests::SelfSerialisingRecord::clear() {
  for (const DoFType* const value : _values) {
    delete value;
  }
  _values.clear();
}

void peano4::grid::tests::SelfSerialisingRecord::startSend(
  Stack<SelfSerialisingRecord>&            stack,
  StackExchangeJob<SelfSerialisingRecord>& job
) {
  const int numberOfElements = static_cast<int>(stack.size());
  assertion(numberOfElements > 0, numberOfElements);

  job.countBuffer.assign(numberOfElements, 0);
  job.dataBuffer.clear();
  for (int element = 0; element < numberOfElements; element++) {
    const SelfSerialisingRecord& record = stack[element];
    job.countBuffer[element]            = static_cast<int>(record.size());
    for (std::size_t index = 0; index < record.size(); index++) {
      job.dataBuffer.push_back(record.at(index));
    }
  }

  [[maybe_unused]] const int countResult =
    tarch::mpi::isend(job.countBuffer.data(), numberOfElements, MPI_INT, job.rank, job.tag, job.comm, &job.requests[0]);
  assertion(countResult == MPI_SUCCESS);

  if (not job.dataBuffer.empty()) {
    [[maybe_unused]] const int dataResult = tarch::mpi::isend(
      job.dataBuffer.data(),
      static_cast<int>(job.dataBuffer.size() * sizeof(DoFType)),
      MPI_BYTE,
      job.rank,
      job.tag,
      job.comm,
      &job.requests[1]
    );
    assertion(dataResult == MPI_SUCCESS);
  }
}

void peano4::grid::tests::SelfSerialisingRecord::startReceive(
  Stack<SelfSerialisingRecord>&            stack,
  StackExchangeJob<SelfSerialisingRecord>& job,
  const int                                numberOfElements
) {
  assertion(numberOfElements > 0, numberOfElements);
  assertion(numberOfElements == static_cast<int>(stack.size()), numberOfElements, stack.size());

  job.countBuffer.assign(numberOfElements, 0);
  job.dataBuffer.clear();

  [[maybe_unused]] const int countResult =
    tarch::mpi::irecv(job.countBuffer.data(), numberOfElements, MPI_INT, job.rank, job.tag, job.comm, &job.requests[0]);
  assertion(countResult == MPI_SUCCESS);
}

/**
 * Drives both phases of one transfer and reports whether it has finished.
 *
 * The payload's length follows from the counts, so a receiver can only post the
 * second message once the first has arrived. The records adopt the values in
 * the same call that completes that second message.
 */
bool peano4::grid::tests::SelfSerialisingRecord::tryToFinishSendOrReceive(
  Stack<SelfSerialisingRecord>&            stack,
  StackExchangeJob<SelfSerialisingRecord>& job
) {
  const bool isReceive = job.mode == StackExchangeJob<SelfSerialisingRecord>::Mode::Receive;

  if (job.requests[0] != MPI_REQUEST_NULL) {
    int completed = 0;
    tarch::mpi::test(&job.requests[0], &completed, MPI_STATUS_IGNORE);
    if (completed == 0) {
      return false;
    }
    job.requests[0] = MPI_REQUEST_NULL;

    if (isReceive) {
      job.dataBuffer.resize(std::accumulate(job.countBuffer.begin(), job.countBuffer.end(), 0));
      if (not job.dataBuffer.empty()) {
        [[maybe_unused]] const int dataResult = tarch::mpi::irecv(
          job.dataBuffer.data(),
          static_cast<int>(job.dataBuffer.size() * sizeof(DoFType)),
          MPI_BYTE,
          job.rank,
          job.tag,
          job.comm,
          &job.requests[1]
        );
        assertion(dataResult == MPI_SUCCESS);
        return false;
      }
    }
  }

  if (job.requests[1] != MPI_REQUEST_NULL) {
    int completed = 0;
    tarch::mpi::test(&job.requests[1], &completed, MPI_STATUS_IGNORE);
    if (completed == 0) {
      return false;
    }
    job.requests[1] = MPI_REQUEST_NULL;
  }

  if (isReceive) {
    std::size_t offset = 0;
    for (std::size_t element = 0; element < stack.size(); element++) {
      // The number of values per element changes between messages, so each
      // record drops its entries before it adopts the incoming ones.
      stack[element] = SelfSerialisingRecord();
      for (int value = 0; value < job.countBuffer[element]; value++) {
        assertion(offset < job.dataBuffer.size(), offset, job.dataBuffer.size());
        stack[element]._values.push_back(new DoFType(job.dataBuffer[offset]));
        offset++;
      }
    }
    assertion(offset == job.dataBuffer.size(), offset, job.dataBuffer.size());
  }

  return true;
}
