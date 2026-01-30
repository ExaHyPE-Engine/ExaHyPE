// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Stack.h"

namespace peano4::stacks {
  template <class T>
  class StackWithCustomSerialisation;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackWithCustomSerialisation: public Stack<T> {
private:
  static tarch::logging::Log _log;

public:
  StackWithCustomSerialisation()                                               = default;
  virtual ~StackWithCustomSerialisation() override                             = default;
  StackWithCustomSerialisation(const StackWithCustomSerialisation&)            = delete;
  StackWithCustomSerialisation& operator=(const StackWithCustomSerialisation&) = delete;

  INLINE_METHOD StackWithCustomSerialisation(StackWithCustomSerialisation&& other) noexcept:
    Stack<T>(std::move(other)) // Calls the base move logic
#ifdef WITH_MPI
    ,
    mpiCountRequest(other.mpiCountRequest),
    mpiDataRequest(other.mpiDataRequest),
    countBuffer(std::move(other.countBuffer)),
    dataBuffer(std::move(other.dataBuffer)),
    mpiComm(other.mpiComm),
    mpiContext(other.mpiContext)
#endif
  {
#ifdef WITH_MPI
    // Reset source handles so two objects don't think they own the same MPI request
    other.mpiCountRequest = MPI_REQUEST_NULL;
    other.mpiDataRequest  = MPI_REQUEST_NULL;
    other.mpiComm         = MPI_COMM_NULL;
#endif
  }

  INLINE_METHOD StackWithCustomSerialisation& operator=(StackWithCustomSerialisation&& other) noexcept {
    if (this != &other) {
      // Call base class move assignment
      Stack<T>::operator=(std::move(other));

#ifdef WITH_MPI
      countBuffer = std::move(other.countBuffer);
      dataBuffer  = std::move(other.dataBuffer);

      mpiCountRequest = other.mpiCountRequest;
      mpiDataRequest  = other.mpiDataRequest;
      mpiComm         = other.mpiComm;
      mpiContext      = other.mpiContext;

      other.mpiCountRequest = MPI_REQUEST_NULL;
      other.mpiDataRequest  = MPI_REQUEST_NULL;
      other.mpiComm         = MPI_COMM_NULL;
#endif
    }
    return *this;
  }

#ifdef WITH_MPI
  MPI_Request mpiCountRequest = MPI_REQUEST_NULL;
  MPI_Request mpiDataRequest  = MPI_REQUEST_NULL;

  std::vector<int>                 countBuffer{};
  std::vector<typename T::DoFType> dataBuffer{};

  MPI_Comm           mpiComm = MPI_COMM_NULL;
  SendReceiveContext mpiContext{};

  void startSend(const SendReceiveContext context, const int rank, const int tag, const MPI_Comm comm) {
    assertion(this->_ioMode == IOMode::None);
    assertion(this->_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(this->size() > 0);
    assertion(not this->_ioMPIActive);

    assertion(this->mpiCountRequest == MPI_REQUEST_NULL);
    assertion(this->mpiDataRequest == MPI_REQUEST_NULL);

    this->_ioMode = IOMode::MPISend;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug("startSend()", this->toString());

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
    assertion3(this->_ioMode == IOMode::None, rank, tag, numberOfElements);
    assertion3(numberOfElements > 0, rank, tag, numberOfElements);
    assertion(this->_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(not this->_ioMPIActive);

    assertion(this->mpiCountRequest == MPI_REQUEST_NULL);
    assertion(this->mpiDataRequest == MPI_REQUEST_NULL);

    this->_ioMode = IOMode::MPIReceive;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug("startReceive()", this->toString());

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
        this->clear();
        logDebug("tryToFinishSendOrReceive()", "Send complete: " << this->toString());
      } else if (this->_ioMode == IOMode::MPIReceive) {
        logDebug("tryToFinishSendOrReceive()", "Receive complete: " << this->toString());
      }
      this->_ioMode         = IOMode::None;
      this->_ioMPIActive    = false;
      this->_ioMPIRequest   = MPI_REQUEST_NULL;
      this->mpiCountRequest = MPI_REQUEST_NULL;
      this->mpiDataRequest  = MPI_REQUEST_NULL;
    }

    return finished;
  }
#endif // WITH_MPI
};

template <class T>
tarch::logging::Log peano4::stacks::StackWithCustomSerialisation<T>::_log(
  "peano4::stacks::StackWithCustomSerialisation<T>"
);
