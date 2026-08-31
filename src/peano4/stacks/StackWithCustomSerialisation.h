// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/IOMode.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "peano4/stacks/Stack.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"

#include <mpi.h>

namespace peano4::stacks {
  template <class T>
  class StackWithCustomSerialisation;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackWithCustomSerialisation final : public peano4::stacks::Stack<T> {
public:
  StackWithCustomSerialisation()                                    = default;
  StackWithCustomSerialisation(const StackWithCustomSerialisation&) = delete;
  INLINE_METHOD                 StackWithCustomSerialisation(StackWithCustomSerialisation&& other) noexcept;
  StackWithCustomSerialisation& operator=(const StackWithCustomSerialisation&) = delete;
  INLINE_METHOD StackWithCustomSerialisation& operator=(StackWithCustomSerialisation&& other) noexcept;
  ~StackWithCustomSerialisation() override = default;

  void startSend(
    const peano4::stacks::SendReceiveContext context,
    const int                                rank,
    const int                                tag,
    const MPI_Comm                           comm
  ) override;

  void startReceive(
    const peano4::stacks::SendReceiveContext context,
    const int                                rank,
    const int                                tag,
    const MPI_Comm                           comm,
    const int                                numberOfElements
  ) override;

  bool tryToFinishSendOrReceive() override;

  /** T::startSend() and T::startReceive() drive the transfer through these requests and buffers. */
  MPI_Request mpiCountRequest{MPI_REQUEST_NULL};
  MPI_Request mpiDataRequest{MPI_REQUEST_NULL};

  std::vector<int>                 countBuffer{};
  std::vector<typename T::DoFType> dataBuffer{};

  MPI_Comm                           mpiComm{MPI_COMM_NULL};
  peano4::stacks::SendReceiveContext mpiContext{};
};

#include "peano4/stacks/StackWithCustomSerialisation.cpph"
