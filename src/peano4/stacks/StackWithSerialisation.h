// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/IOMode.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "peano4/stacks/Stack.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

namespace peano4::stacks {
  template <class T>
  class StackWithSerialisation;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackWithSerialisation final : public peano4::stacks::Stack<T> {
public:
  static_assert(T::Cardinality > 0);
  static_assert(std::is_trivial_v<typename T::DoFType>);
  static_assert(std::is_standard_layout_v<typename T::DoFType>);

  StackWithSerialisation()                                             = default;
  StackWithSerialisation(const StackWithSerialisation&)                = delete;
  StackWithSerialisation(StackWithSerialisation&&) noexcept            = default;
  StackWithSerialisation& operator=(const StackWithSerialisation&)     = delete;
  StackWithSerialisation& operator=(StackWithSerialisation&&) noexcept = default;
  ~StackWithSerialisation() override                                   = default;

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

private:
  std::vector<typename T::DoFType> _sendBuffer{};
  std::vector<typename T::DoFType> _receiveBuffer{};

  void serialise();
  void deserialise();
};

#include "peano4/stacks/StackWithSerialisation.cpph"
