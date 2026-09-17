// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/Stack.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

namespace peano4::grid {
  template <class T>
  struct StackExchangeBufferElement;

  template <class T>
  struct StackExchangeJob;

  /**
   * Identifies element types that provide their own stack-exchange protocol.
   */
  template <class T>
  concept SerialisesItself = requires(Stack<T>& stack, StackExchangeJob<T>& job, int count) {
    T::startSend(stack, job);
    T::startReceive(stack, job, count);
    { T::tryToFinishSendOrReceive(stack, job) } -> std::convertible_to<bool>;
  };

  template <class T>
  void startSend(Stack<T>& stack, StackExchangeJob<T>& job, int rank, int tag, MPI_Comm comm);

  template <class T>
  void startReceive(Stack<T>& stack, StackExchangeJob<T>& job, int rank, int tag, MPI_Comm comm, int numberOfElements);

  template <class T>
  bool tryToFinishSendOrReceive(Stack<T>& stack, StackExchangeJob<T>& job);
} // namespace peano4::grid

/**
 * Selects the scratch-buffer element type for a custom exchange protocol.
 */
template <class T>
struct peano4::grid::StackExchangeBufferElement {
  using Type = char;
};

template <class T>
  requires requires { typename T::DoFType; }
struct peano4::grid::StackExchangeBufferElement<T> {
  using Type = typename T::DoFType;
};

/**
 * Stores transport state for one non-blocking stack send or receive. A custom
 * protocol's T::startSend() and T::startReceive() drive the transfer through
 * these requests and buffers.
 */
template <class T>
struct peano4::grid::StackExchangeJob final {
  enum class Mode { Send, Receive };

  Mode     mode{Mode::Send};
  int      rank{-1};
  int      tag{-1};
  MPI_Comm comm{MPI_COMM_NULL};
  //! Stores count and payload requests; direct exchange uses the first slot.
  MPI_Request                                               requests[2]{MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  std::vector<int>                                          countBuffer{};
  std::vector<typename StackExchangeBufferElement<T>::Type> dataBuffer{};
};

/**
 * Starts a non-blocking stack send through a custom protocol or raw bytes.
 */
template <class T>
void peano4::grid::startSend(
  Stack<T>&            stack,
  StackExchangeJob<T>& job,
  const int            rank,
  const int            tag,
  const MPI_Comm       comm
) {
  assertion(stack.size() > 0);
  assertion(job.requests[0] == MPI_REQUEST_NULL);
  assertion(job.requests[1] == MPI_REQUEST_NULL);
  job.mode = StackExchangeJob<T>::Mode::Send;
  job.rank = rank;
  job.tag  = tag;
  job.comm = comm;
  if constexpr (SerialisesItself<T>) {
    T::startSend(stack, job);
  } else {
    static_assert(
      std::is_trivially_copyable_v<T>,
      "The direct exchange path ships raw bytes, so an element type without protocol statics has to be trivially "
      "copyable."
    );
    constexpr auto MaximumMessageBytes = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (stack.size() > MaximumMessageBytes / sizeof(T)) {
      tarch::terminateWithError("An MPI stack message exceeds the supported 32-bit byte count.");
    }
    const auto                 messageBytes = static_cast<int>(stack.size() * sizeof(T));
    [[maybe_unused]] const int result =
      tarch::mpi::isend(stack.data(), messageBytes, MPI_BYTE, rank, tag, comm, &job.requests[0]);
    assertion(result == MPI_SUCCESS, rank, tag);
  }
}

/**
 * Starts a non-blocking stack receive through a custom protocol or raw bytes.
 */
template <class T>
void peano4::grid::startReceive(
  Stack<T>&            stack,
  StackExchangeJob<T>& job,
  const int            rank,
  const int            tag,
  const MPI_Comm       comm,
  const int            numberOfElements
) {
  assertion(numberOfElements > 0, rank, tag);
  assertion(job.requests[0] == MPI_REQUEST_NULL);
  assertion(job.requests[1] == MPI_REQUEST_NULL);
  job.mode = StackExchangeJob<T>::Mode::Receive;
  job.rank = rank;
  job.tag  = tag;
  job.comm = comm;
  if constexpr (not SerialisesItself<T>) {
    constexpr auto MaximumMessageBytes = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (static_cast<std::size_t>(numberOfElements) > MaximumMessageBytes / sizeof(T)) {
      tarch::terminateWithError("An MPI stack message exceeds the supported 32-bit byte count.");
    }
  }
  stack.resize(static_cast<std::size_t>(numberOfElements));
  if constexpr (SerialisesItself<T>) {
    T::startReceive(stack, job, numberOfElements);
  } else {
    static_assert(
      std::is_trivially_copyable_v<T>,
      "The direct exchange path ships raw bytes, so an element type without protocol statics has to be trivially "
      "copyable."
    );
    const auto                 messageBytes = static_cast<int>(static_cast<std::size_t>(numberOfElements) * sizeof(T));
    [[maybe_unused]] const int result =
      tarch::mpi::irecv(stack.data(), messageBytes, MPI_BYTE, rank, tag, comm, &job.requests[0]);
    assertion(result == MPI_SUCCESS, rank, tag);
  }
}

/**
 * Polls a request and releases a completed send stack for reuse.
 */
template <class T>
bool peano4::grid::tryToFinishSendOrReceive(Stack<T>& stack, StackExchangeJob<T>& job) {
  bool finished;
  if constexpr (SerialisesItself<T>) {
    finished = T::tryToFinishSendOrReceive(stack, job);
  } else {
    int                        flag   = 0;
    [[maybe_unused]] const int result = tarch::mpi::test(&job.requests[0], &flag, MPI_STATUS_IGNORE);
    assertion(result == MPI_SUCCESS, job.rank, job.tag);
    finished = flag != 0;
  }
  if (finished and job.mode == StackExchangeJob<T>::Mode::Send) {
    stack.shallowClear();
  }
  return finished;
}
