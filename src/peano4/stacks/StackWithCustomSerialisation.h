// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/Stack.h"

#include <mpi.h>

namespace peano4::stacks {
  template <class T>
  class StackWithCustomSerialisation;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackWithCustomSerialisation : public Stack<T> {
public:
  StackWithCustomSerialisation()                                               = default;
  ~StackWithCustomSerialisation() override                                     = default;
  StackWithCustomSerialisation(const StackWithCustomSerialisation&)            = delete;
  StackWithCustomSerialisation& operator=(const StackWithCustomSerialisation&) = delete;

  INLINE_METHOD StackWithCustomSerialisation(StackWithCustomSerialisation&& other) noexcept :
    Stack<T>(std::move(other)),
    mpiCountRequest(other.mpiCountRequest),
    mpiDataRequest(other.mpiDataRequest),
    countBuffer(std::move(other.countBuffer)),
    dataBuffer(std::move(other.dataBuffer)),
    mpiComm(other.mpiComm),
    mpiContext(other.mpiContext) {
    other.mpiCountRequest = MPI_REQUEST_NULL;
    other.mpiDataRequest  = MPI_REQUEST_NULL;
    other.mpiComm         = MPI_COMM_NULL;
  }

  INLINE_METHOD StackWithCustomSerialisation& operator=(StackWithCustomSerialisation&& other) noexcept {
    if (this != &other) {
      Stack<T>::operator=(std::move(other));

      countBuffer = std::move(other.countBuffer);
      dataBuffer  = std::move(other.dataBuffer);

      mpiCountRequest = other.mpiCountRequest;
      mpiDataRequest  = other.mpiDataRequest;
      mpiComm         = other.mpiComm;
      mpiContext      = other.mpiContext;

      other.mpiCountRequest = MPI_REQUEST_NULL;
      other.mpiDataRequest  = MPI_REQUEST_NULL;
      other.mpiComm         = MPI_COMM_NULL;
    }
    return *this;
  }

  MPI_Request mpiCountRequest{MPI_REQUEST_NULL};
  MPI_Request mpiDataRequest{MPI_REQUEST_NULL};

  std::vector<int>                 countBuffer{};
  std::vector<typename T::DoFType> dataBuffer{};

  MPI_Comm           mpiComm{MPI_COMM_NULL};
  SendReceiveContext mpiContext{};

  void startSend(const SendReceiveContext context, const int rank, const int tag, const MPI_Comm comm) {
    assertion(this->_ioMode == IOMode::None);
    assertion(this->size() > 0);
    assertion(not this->_ioMPIActive);

    assertion(this->mpiCountRequest == MPI_REQUEST_NULL);
    assertion(this->mpiDataRequest == MPI_REQUEST_NULL);

    this->_ioMode = IOMode::MPISend;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug(this->toString());

    T::startSend(*this, context, comm);

    this->_ioMPIActive = true;
  }

  void startReceive(
    const SendReceiveContext context,
    const int                rank,
    const int                tag,
    const MPI_Comm           comm,
    const int                numberOfElements
  ) {
    assertion(this->_ioMode == IOMode::None, rank, tag, numberOfElements);
    assertion(numberOfElements > 0, rank, tag, numberOfElements);
    assertion(not this->_ioMPIActive);

    assertion(this->mpiCountRequest == MPI_REQUEST_NULL);
    assertion(this->mpiDataRequest == MPI_REQUEST_NULL);

    this->_ioMode = IOMode::MPIReceive;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug(this->toString());

    this->resize(numberOfElements);

    T::startReceive(*this, context, comm, numberOfElements);

    this->_ioMPIActive = true;
  }

  bool tryToFinishSendOrReceive() {
    if (this->_ioMode == IOMode::None or not this->_ioMPIActive) {
      return true;
    }

    const bool finished = T::tryToFinishSendOrReceive(*this);

    if (finished) {
      if (this->_ioMode == IOMode::MPISend) {
        this->shallowClear();
        logDebug("Send complete: " << this->toString());
      } else if (this->_ioMode == IOMode::MPIReceive) {
        logDebug("Receive complete: " << this->toString());
      }
      this->_ioMode         = IOMode::None;
      this->_ioMPIActive    = false;
      this->mpiCountRequest = MPI_REQUEST_NULL;
      this->mpiDataRequest  = MPI_REQUEST_NULL;
    }

    return finished;
  }
};
