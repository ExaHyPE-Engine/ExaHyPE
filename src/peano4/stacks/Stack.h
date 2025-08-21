// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "IOMode.h"
#include "peano4/grid/TraversalObserver.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"

#include <utility>
#include <vector>

namespace peano4::stacks {
  template <class T>
  class Stack;

#if defined(WITH_MPI)
  template <class T>
  MPI_Datatype translateContextIntoDatatype(grid::TraversalObserver::SendReceiveContext context) {
    switch (context) {
    case grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      return T::getBoundaryExchangeDatatype();
    case grid::TraversalObserver::SendReceiveContext::MultiscaleExchange:
      return T::getMultiscaleDataExchangeDatatype();
    case grid::TraversalObserver::SendReceiveContext::ForkDomain:
      return T::getForkDatatype();
    case grid::TraversalObserver::SendReceiveContext::JoinDomain:
      return T::getJoinDatatype();
    case grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertionMsg(false, "Periodic boundary data is never exchanged via MPI");
      return T::getGlobalCommunicationDatatype();
    }
    assertionMsg(false, "No valid case statement found. This may not happen");
    return T::getGlobalCommunicationDatatype();
  }
#endif
} // namespace peano4::stacks

template <class T>
class peano4::stacks::Stack {
private:
  static tarch::logging::Log _log;

protected:
  std::vector<T> _data;

  IOMode _ioMode = IOMode::None;
  int    _ioRank = -1;
  int    _ioTag  = -1;
#ifdef WITH_MPI
  MPI_Request _ioMPIRequest = MPI_REQUEST_NULL;
  bool        _ioMPIActive  = false;
#endif

public:
  Stack()                         = default;
  virtual ~Stack()                = default;
  Stack(const Stack&)             = delete;
  Stack& operator=(const Stack&)  = delete;
  Stack(Stack&&)                  = delete;
  Stack& operator=(Stack&&)       = delete;

  std::size_t push(const T& element) {
    _data.push_back(element); // Copy element into std::vector
    return _data.size() - 1;
  }

  std::size_t push(T&& element) {
    // std::move is required here to make sure it is actually moved (as the argument has a name)
    _data.push_back(std::move(element)); // Move element into std::vector
    return _data.size() - 1;
  }

  /// Emplaces a new element in-place.
  template <typename... Args>
  std::size_t emplace(Args&&... args) {
    _data.emplace_back(std::forward<Args>(args)...);
    return _data.size() - 1;
  }

  /// Erases the element at the given index.
  /// The removed element is replaced with the last element (order is not preserved).
  void erase(std::size_t index) {
    assertion(index < _data.size());
    _data[index] = std::move(_data.back());
    _data.pop_back();
  }

  void reserve(const std::size_t size) { _data.reserve(size); }

  void resize(const std::size_t size) {
    if (_data.size() < size) {
      // Grow
      _data.reserve(size);
      while (_data.size() < size) {
        _data.push_back(T());
      }
    } else if (_data.size() > size) {
      // Shrink
      while (_data.size() > size) {
        _data.pop_back();
      }
    }
  }

  /// Returns a reference to the element at the given index.
  T& operator[](std::size_t index) {
    assertion(index < _data.size());
    return _data[index];
  }

  const T& operator[](std::size_t index) const {
    assertion(index < _data.size());
    return _data[index];
  }

  /// Removes and returns the last element.
  T pop() {
    assertion(!_data.empty());
    T element = std::move(_data.back());
    _data.pop_back();
    return element;
  }

  void clone(const Stack<T>& stack) {
    assertion(this->empty());
    for (const T& element : stack._data) {
      _data.emplace_back(element); // Uses T's copy constructor
    }
  }

  std::string toString() const {
    std::ostringstream msg;
    msg << "Stack(Size=" << size();
#ifdef WITH_MPI
    msg << ",Mode=" << ::toString(_ioMode) << ",Rank=" << _ioRank << ",Tag=" << _ioTag;
#else
    msg << ",Mode=None,Rank=-1,Tag=-1";
#endif
    msg << ")";
    return msg.str();
  }

  /// Returns the current number of elements.
  std::size_t size() const { return _data.size(); }
  /// Checks if the container is empty.
  bool empty() const { return _data.empty(); }
  /// Clears all stored elements.
  void clear() { _data.clear(); }

#ifdef WITH_MPI
  /**
   * Always pairs up with a finish... call. When we trigger the send, the
   * code still might insert additional elements, i.e. starting does not
   * mean all the data that is to be sent out is already in the container.
   *
   * @param context Determines which MPI datatype to use.
   * @param rank Destination rank.
   * @param tag MPI tag.
   * @param comm MPI communicator.
   */
  void startSend(
    const grid::TraversalObserver::SendReceiveContext context,
    const int                                         rank,
    const int                                         tag,
    const MPI_Comm                                    comm
  ) {
    assertion(_ioMode == IOMode::None);
    assertion(_ioMPIRequest == MPI_REQUEST_NULL);

    _ioMode = IOMode::MPISend;
    _ioTag  = tag;
    _ioRank = rank;

    logDebug("startSend()", toString());

    const MPI_Datatype dtype  = translateContextIntoDatatype<T>(context);
    const int          result = MPI_Isend(
      _data.data(),
      static_cast<int>(_data.size()),
      dtype,
      _ioRank,
      _ioTag,
      comm,
      &_ioMPIRequest
    );
    if (result != MPI_SUCCESS) {
      logError(
        "startSend()",
        "Was not able to send to rank "
          << _ioRank << " on tag " << _ioTag << ": " << tarch::mpi::MPIReturnValueToString(result)
      );
      assertion(false);
    }
    _ioMPIActive = true;
  }

  /**
   * Initiates a non-blocking receive into the container. @see startSend()
   *
   * @param context Determines which MPI datatype to use.
   * @param rank Source rank.
   * @param tag MPI tag.
   * @param comm MPI communicator.
   * @param numberOfElements Number of elements expected.
   */
  void startReceive(
    const grid::TraversalObserver::SendReceiveContext context,
    const int                                         rank,
    const int                                         tag,
    const MPI_Comm                                    comm,
    const int                                         numberOfElements
  ) {
    assertion3(_ioMode == IOMode::None, rank, tag, numberOfElements);
    assertion3(numberOfElements > 0, rank, tag, numberOfElements);

    _ioMode = IOMode::MPIReceive;
    _ioTag  = tag;
    _ioRank = rank;

    logDebug("startReceive()", toString());

    resize(numberOfElements);
    const MPI_Datatype dtype  = translateContextIntoDatatype<T>(context);
    const int          result = MPI_Irecv(_data.data(), numberOfElements, dtype, _ioRank, _ioTag, comm, &_ioMPIRequest);
    if (result != MPI_SUCCESS) {
      logError(
        "startReceive()",
        "Was not able to receive "
          << numberOfElements << " values from rank " << _ioRank << " on tag " << _ioTag << ": "
          << tarch::mpi::MPIReturnValueToString(result)
      );
      assertion(false);
    }
    _ioMPIActive = true;
  }

  /**
   * If this routine is invoked on a stack which is neither sending or
   * receiving, then it degenerates to nop. If it is a receiving stack,
   * then the routine waits until all of the data is literally in. If we
   * invoke it on a sending stack, the code checks that the send is
   * complete, i.e. the data is either received or at least somewhere in
   * the network. After that, it releases the underlying stack, i.e.
   * clears it. Once this operation has terminated, the stack's state will
   * be IOMode::None.
   *
   * @return finished send/receive already.
   */
  bool tryToFinishSendOrReceive() {
    if (_ioMode == IOMode::None or not _ioMPIActive) {
      return true;
    }

    int flag = 0;
    MPI_Test(&_ioMPIRequest, &flag, MPI_STATUS_IGNORE);
    if (flag) {
      if (_ioMode == IOMode::MPISend) {
        clear();
        logDebug("tryToFinishSendOrReceive()", "Send complete: " << toString());
      } else if (_ioMode == IOMode::MPIReceive) {
        logDebug("tryToFinishSendOrReceive()", "Receive complete: " << toString());
      }
      _ioMode      = IOMode::None;
      _ioMPIActive = false;

      return true;
    }

    return false;
  }

  /**
   * Returns the rank of the communication partner if a send/receive is active,
   * or -1 otherwise.
   */
  int getActiveIORank() const { return _ioMode == IOMode::None ? -1 : _ioRank; }
#endif // WITH_MPI
};

template <class T>
tarch::logging::Log peano4::stacks::Stack<T>::_log("peano4::stacks::Stack<T>");
