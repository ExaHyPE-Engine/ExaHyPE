// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/Stack.h"
#include "tarch/mpi/mpi.h"

namespace peano4::stacks {
  template <class T>
  class StackWithSerialisation;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackWithSerialisation : public Stack<T> {
private:
  static_assert(T::Cardinality > 0);
  static_assert(std::is_trivial_v<typename T::DoFType>);
  static_assert(std::is_standard_layout_v<typename T::DoFType>);

  std::vector<typename T::DoFType> _sendBuffer;
  std::vector<typename T::DoFType> _receiveBuffer;

public:
  StackWithSerialisation()                                             = default;
  virtual ~StackWithSerialisation() override                           = default;
  StackWithSerialisation(const StackWithSerialisation&)                = delete;
  StackWithSerialisation& operator=(const StackWithSerialisation&)     = delete;
  StackWithSerialisation(StackWithSerialisation&&) noexcept            = default;
  StackWithSerialisation& operator=(StackWithSerialisation&&) noexcept = default;

#ifdef PEANO_WITH_MPI
  void startSend(const SendReceiveContext context, const int rank, const int tag, const MPI_Comm comm) {
    assertion(this->_ioMode == IOMode::None);
    assertion(this->_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(this->size() > 0);
    assertion(not this->_ioMPIActive);

    this->_ioMode = IOMode::MPISend;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug(this->toString());

    _sendBuffer.resize(this->size() * T::Cardinality);
    serialise();

    const MPI_Datatype dtype  = translateContextIntoDatatype<T>(context);
    const int          result = tarch::mpi::isend(
      _sendBuffer.data(),
      static_cast<int>(_sendBuffer.size()),
      dtype,
      this->_ioRank,
      this->_ioTag,
      comm,
      &this->_ioMPIRequest
    );
    if (result != MPI_SUCCESS) {
      logError(
        "Was not able to send to rank " << this->_ioRank << " on tag " << this->_ioTag << ": "
                                        << tarch::mpi::MPIReturnValueToString(result)
      );
      assertion(false);
    }

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
    assertion(this->_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(not this->_ioMPIActive);

    this->_ioMode = IOMode::MPIReceive;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug(this->toString());

    this->resize(numberOfElements);
    _receiveBuffer.resize(numberOfElements * T::Cardinality);

    const MPI_Datatype dtype  = translateContextIntoDatatype<T>(context);
    const int          result = tarch::mpi::irecv(
      _receiveBuffer.data(),
      static_cast<int>(_receiveBuffer.size()),
      dtype,
      this->_ioRank,
      this->_ioTag,
      comm,
      &this->_ioMPIRequest
    );
    if (result != MPI_SUCCESS) {
      logError(
        "Was not able to receive " << numberOfElements << " values from rank " << this->_ioRank << " on tag "
                                   << this->_ioTag << ": " << tarch::mpi::MPIReturnValueToString(result)
      );
      assertion(false);
    }

    this->_ioMPIActive = true;
  }

  bool tryToFinishSendOrReceive() {
    if (this->_ioMode == IOMode::None or not this->_ioMPIActive) {
      return true;
    }

    int flag = 0;
    tarch::mpi::test(&this->_ioMPIRequest, &flag, MPI_STATUS_IGNORE);
    if (flag) {
      if (this->_ioMode == IOMode::MPISend) {
        this->shallowClear();
        logDebug("Send complete: " << this->toString());
      } else if (this->_ioMode == IOMode::MPIReceive) {
        deserialise();
        logDebug("Receive complete: " << this->toString());
      }
      this->_ioMode       = IOMode::None;
      this->_ioMPIActive  = false;
      this->_ioMPIRequest = MPI_REQUEST_NULL;

      return true;
    }

    return false;
  }

private:
  void serialise() {
    for (std::size_t i = 0; i < this->size(); ++i) {
      // Use non-const data(): generated lazy patch data materialises
      // deterministic storage here instead of exposing stale reused slots.
      typename T::DoFType* source = this->_data[i].data();
      assertion(source != nullptr);
      std::copy_n(source, T::Cardinality, _sendBuffer.data() + i * T::Cardinality);
    }
  }

  void deserialise() {
    for (std::size_t i = 0; i < this->size(); ++i) {
      // Use non-const data(): generated lazy patch data materialises receive
      // storage on demand before the incoming payload is copied into it.
      typename T::DoFType* target = this->_data[i].data();
      if (target == nullptr) {
        this->_data[i] = T();
        target         = this->_data[i].data();
      }
      assertion(target != nullptr);
      std::copy_n(_receiveBuffer.data() + i * T::Cardinality, T::Cardinality, target);
    }
  }
#endif // PEANO_WITH_MPI
};
