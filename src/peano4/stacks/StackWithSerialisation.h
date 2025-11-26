// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Stack.h"

namespace peano4::stacks {
  template <class T>
  class StackWithSerialisation;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::StackWithSerialisation: public Stack<T> {
private:
  static tarch::logging::Log _log;

  static_assert(T::Cardinality > 0);
  static_assert(std::is_trivial_v<typename T::DoFType>);
  static_assert(std::is_standard_layout_v<typename T::DoFType>);

  std::vector<typename T::DoFType> _sendBuffer;
  std::vector<typename T::DoFType> _receiveBuffer;

public:
  StackWithSerialisation()                                         = default;
  virtual ~StackWithSerialisation() override                       = default;
  StackWithSerialisation(const StackWithSerialisation&)            = delete;
  StackWithSerialisation& operator=(const StackWithSerialisation&) = delete;
  StackWithSerialisation(StackWithSerialisation&&)                 = delete;
  StackWithSerialisation& operator=(StackWithSerialisation&&)      = delete;

#ifdef WITH_MPI
  void startSend(const SendReceiveContext context, const int rank, const int tag, const MPI_Comm comm) {
    assertion(this->_ioMode == IOMode::None);
    assertion(this->_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(this->size() > 0);
    assertion(not this->_ioMPIActive);

    this->_ioMode = IOMode::MPISend;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug("startSend()", this->toString());

    _sendBuffer.resize(this->size() * T::Cardinality);
    serialise();

    const MPI_Datatype dtype  = translateContextIntoDatatype<T>(context);
    const int          result = MPI_Isend(
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
        "startSend()",
        "Was not able to send to rank "
          << this->_ioRank << " on tag " << this->_ioTag << ": " << tarch::mpi::MPIReturnValueToString(result)
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
    assertion3(this->_ioMode == IOMode::None, rank, tag, numberOfElements);
    assertion3(numberOfElements > 0, rank, tag, numberOfElements);
    assertion(this->_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(not this->_ioMPIActive);

    this->_ioMode = IOMode::MPIReceive;
    this->_ioTag  = tag;
    this->_ioRank = rank;

    logDebug("startReceive()", this->toString());

    this->resize(numberOfElements);
    _receiveBuffer.resize(numberOfElements * T::Cardinality);

    const MPI_Datatype dtype  = translateContextIntoDatatype<T>(context);
    const int          result = MPI_Irecv(
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
        "startReceive()",
        "Was not able to receive "
          << numberOfElements << " values from rank " << this->_ioRank << " on tag " << this->_ioTag << ": "
          << tarch::mpi::MPIReturnValueToString(result)
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
    MPI_Test(&this->_ioMPIRequest, &flag, MPI_STATUS_IGNORE);
    if (flag) {
      if (this->_ioMode == IOMode::MPISend) {
        this->clear();
        logDebug("tryToFinishSendOrReceive()", "Send complete: " << this->toString());
      } else if (this->_ioMode == IOMode::MPIReceive) {
        deserialise();
        logDebug("tryToFinishSendOrReceive()", "Receive complete: " << this->toString());
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
      std::copy_n(this->_data[i].data(), T::Cardinality, _sendBuffer.data() + i * T::Cardinality);
    }
  }

  void deserialise() {
    for (std::size_t i = 0; i < this->size(); ++i) {
      // if (this->_data[i].data() == nullptr) {
      //   this->_data[i] = T(); // Default-construct allocates
      // }
      std::copy_n(_receiveBuffer.data() + i * T::Cardinality, T::Cardinality, this->_data[i].data());
    }
  }
#endif // WITH_MPI
};

template <class T>
tarch::logging::Log peano4::stacks::StackWithSerialisation<T>::_log("peano4::stacks::StackWithSerialisation<T>");
