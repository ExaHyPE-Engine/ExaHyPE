// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "IOMode.h"
#include "SendReceiveContext.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"

#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace peano4::stacks {
  template <class T>
  class Stack;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::Stack {
private:
  static tarch::logging::Log _log;

  // static_assert(std::is_standard_layout_v<T>);
  // static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::is_copy_constructible_v<T>);

protected:
  std::vector<T> _data{};
  std::size_t    _logicalSize = 0; // Number of logically used elements (<= _data.size())

  IOMode _ioMode = IOMode::None;
  int    _ioRank = -1;
  int    _ioTag  = -1;
#ifdef WITH_MPI
  MPI_Request _ioMPIRequest = MPI_REQUEST_NULL;
  bool        _ioMPIActive  = false;
#endif

  class View {
  private:
    friend class Stack<T>;
    const std::size_t _size;
    const std::size_t _baseElement;
    Stack<T>*         _stack;

    INLINE_METHOD View(std::size_t size, std::size_t base, Stack<T>* stack):
      _size(size),
      _baseElement(base),
      _stack(stack) {}

  public:
    INLINE_METHOD std::size_t size() const { return _size; }

    INLINE_METHOD T& get(std::size_t index) {
      assertion2(index < _size, index, _size);
      return _stack->_data[_baseElement + index];
    }

    INLINE_METHOD const T& get(std::size_t index) const {
      assertion2(index < _size, index, _size);
      return _stack->_data[_baseElement + index];
    }

    INLINE_METHOD void set(std::size_t index, T&& value) {
      assertion2(index < _size, index, _size);
      _stack->_data[_baseElement + index] = std::move(value);
    }

    // INLINE_METHOD void set(std::size_t index, const T& value) {
    //   assertion2(index < _size, index, _size);
    //   _stack->_data[_baseElement + index] = T(value);
    // }

    // template <class... Args>
    // INLINE_METHOD void emplace(std::size_t index, Args&&... args) {
    //   assertion2(index < _size, index, _size);
    //   _stack->_data[_baseElement + index] = T(std::forward<Args>(args)...);
    // }

    // In-place reconstruct the element at index.
    // Destroys the current object, then constructs a new one in the same slot.
    template <class... Args>
    INLINE_METHOD void emplace(std::size_t index, Args&&... args) noexcept(
      std::is_nothrow_destructible_v<T> && std::is_nothrow_constructible_v<T, Args...>
    ) {
      assertion2(index < _size, index, _size);
      T* slot = &_stack->_data[_baseElement + index];
      std::destroy_at(slot);
      std::construct_at(slot, std::forward<Args>(args)...);
    }

    INLINE_METHOD void emplace_default(
      std::size_t index
    ) noexcept(std::is_nothrow_destructible_v<T> && std::is_nothrow_default_constructible_v<T>) {
      emplace(index);
    }

    INLINE_METHOD void emplace_no_data(std::size_t index) noexcept(
      std::is_nothrow_destructible_v<T> && std::is_nothrow_constructible_v<T, typename T::ObjectConstruction>
    ) {
      emplace(index, T::ObjectConstruction::NoData);
    }
  };

public:
  INLINE_METHOD Stack() noexcept = default;
  virtual ~Stack() noexcept      = default;
  Stack(const Stack&)            = delete;
  Stack& operator=(const Stack&) = delete;

  INLINE_METHOD Stack(Stack&& other) noexcept:
    _data(std::move(other._data)),
    _logicalSize(other._logicalSize),
    _ioMode(other._ioMode),
    _ioRank(other._ioRank),
    _ioTag(other._ioTag)
#ifdef WITH_MPI
    ,
    _ioMPIRequest(other._ioMPIRequest),
    _ioMPIActive(other._ioMPIActive)
#endif
  {
    // Reset the source to a safe empty state
    other._logicalSize = 0;
    other._ioMode      = IOMode::None;
#ifdef WITH_MPI
    other._ioMPIRequest = MPI_REQUEST_NULL;
    other._ioMPIActive  = false;
#endif
  }

  INLINE_METHOD Stack& operator=(Stack&& other) noexcept {
    if (this != &other) {
      _data        = std::move(other._data);
      _logicalSize = other._logicalSize;
      _ioMode      = other._ioMode;
      _ioRank      = other._ioRank;
      _ioTag       = other._ioTag;
#ifdef WITH_MPI
      _ioMPIRequest = other._ioMPIRequest;
      _ioMPIActive  = other._ioMPIActive;

      other._ioMPIRequest = MPI_REQUEST_NULL;
      other._ioMPIActive  = false;
#endif
      other._logicalSize = 0;
      other._ioMode      = IOMode::None;
    }
    return *this;
  }

  INLINE_METHOD std::size_t push(const T& element) {
    if (_logicalSize < _data.size()) {
      _data[_logicalSize] = element;
      // _data[_logicalSize] = T(element); // Copy-construct temp, then move-assign
    } else {
      _data.push_back(element); // Copy element into std::vector
    }
    return _logicalSize++;
  }

  INLINE_METHOD std::size_t push(T&& element) { // May not be const
    // std::move is required here to make sure it is actually moved (as the argument has a name)
    if (_logicalSize < _data.size()) {
      _data[_logicalSize] = std::move(element);
    } else {
      _data.push_back(std::move(element)); // Move element into std::vector
    }
    return _logicalSize++;
  }

  /// Emplaces a new element in-place.
  template <typename... Args>
  INLINE_METHOD std::size_t emplace(Args&&... args) {
    if (_logicalSize < _data.size()) {
      _data[_logicalSize] = T(std::forward<Args>(args)...);
    } else {
      _data.emplace_back(std::forward<Args>(args)...);
    }
    return _logicalSize++;
  }

  /// Erases the element at the given index.
  /// The removed element is replaced with the last element (order is not preserved).
  INLINE_METHOD void erase(std::size_t index) {
    assertion(index < _logicalSize);
    // Move last logical element into the hole, then shrink logical size
    _data[index] = std::move(_data[_logicalSize - 1]);
    --_logicalSize;
  }

  INLINE_METHOD void reserve(const std::size_t size) { _data.reserve(size); }

  INLINE_METHOD void resize(const std::size_t size) {
    if (size <= _data.size()) {
      // We already have enough storage; just move the logical boundary
      _logicalSize = size;
    } else {
      // Need more physical storage; construct new elements
      //_data.resize(size, T(T::ObjectConstruction::NoData));
      _data.reserve(size);
      while (_data.size() < size) {
        _data.emplace_back(); // Construct in place, no assignment
      }
      _logicalSize = size;
    }
  }

  /// Returns a reference to the element at the given index.
  INLINE_METHOD T& operator[](std::size_t index) {
    assertion(index < _logicalSize);
    return _data[index];
  }

  INLINE_METHOD const T& operator[](std::size_t index) const {
    assertion(index < _logicalSize);
    return _data[index];
  }

  /// Removes and returns the last element.
  INLINE_METHOD T pop() {
    assertion(_logicalSize > 0);
    T element = std::move(_data[_logicalSize - 1]);
    --_logicalSize;
    return element;
  }

  /// Get top element with shift (0 = top, 1 = next, etc.)
  INLINE_METHOD T& top(std::size_t shift = 0) {
    assertion(_logicalSize > 0);
    assertion(shift < _logicalSize);
    return _data[_logicalSize - 1 - shift];
  }
  INLINE_METHOD const T& top(std::size_t shift = 0) const {
    assertion(_logicalSize > 0);
    assertion(shift < _logicalSize);
    return _data[_logicalSize - 1 - shift];
  }

  INLINE_METHOD View popBlock(std::size_t numberOfElements) {
    assertion(numberOfElements <= _logicalSize);
    const std::size_t base = _logicalSize - numberOfElements;
    View              view(numberOfElements, base, this);
    _logicalSize = base;
    return view;
  }

  INLINE_METHOD View pushBlock(std::size_t numberOfElements) {
    const std::size_t base = _logicalSize;
    _logicalSize += numberOfElements;
    if (_data.size() < _logicalSize) {
      //_data.resize(_logicalSize, T(T::ObjectConstruction::NoData));
      _data.reserve(_logicalSize);
      while (_data.size() < _logicalSize) {
        _data.emplace_back(T::ObjectConstruction::NoData); // Construct in place
      }
    }
    return View(numberOfElements, base, this);
  }

  INLINE_METHOD void clone(const Stack<T>& other) {
    assertion(this->empty());
    const std::size_t n = other._logicalSize;
    _data.clear();
    _data.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      _data.emplace_back(other._data[i]); // Copy-construct each live element
    }
    _logicalSize = n;
  }

  std::string toString() const {
    std::ostringstream msg;
    msg << "Stack{";
    msg << "Size=" << size() << ", Mode=" << ::toString(_ioMode) << ", Rank=" << _ioRank << ", Tag=" << _ioTag;
    msg << "}";
    return msg.str();
  }

  INLINE_METHOD std::size_t size() const noexcept { return _logicalSize; }

  INLINE_METHOD bool empty() const noexcept { return _logicalSize == 0; }

  INLINE_METHOD void clear() {
    shallowClear();
    _data.clear();
  }

  INLINE_METHOD void shallowClear() { _logicalSize = 0; }

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
  void startSend(const SendReceiveContext context, const int rank, const int tag, const MPI_Comm comm) {
    assertion(_ioMode == IOMode::None);
    assertion(_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(size() > 0);
    assertion(not _ioMPIActive);

    _ioMode = IOMode::MPISend;
    _ioTag  = tag;
    _ioRank = rank;

    logDebug("startSend()", toString());

    const MPI_Datatype dtype = translateContextIntoDatatype<T>(context);
    const int result = MPI_Isend(_data.data(), static_cast<int>(size()), dtype, _ioRank, _ioTag, comm, &_ioMPIRequest);
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
    const SendReceiveContext context,
    const int                rank,
    const int                tag,
    const MPI_Comm           comm,
    const int                numberOfElements
  ) {
    assertion3(_ioMode == IOMode::None, rank, tag, numberOfElements);
    assertion3(numberOfElements > 0, rank, tag, numberOfElements);
    assertion(_ioMPIRequest == MPI_REQUEST_NULL);
    assertion(not _ioMPIActive);

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
      _ioMode       = IOMode::None;
      _ioMPIActive  = false;
      _ioMPIRequest = MPI_REQUEST_NULL;

      return true;
    }

    return false;
  }

  /**
   * Returns the rank of the communication partner if a send/receive is active,
   * or -1 otherwise.
   */
  INLINE_METHOD IOMode getIOMode() const { return _ioMode; }
  INLINE_METHOD int    getActiveIORank() const { return _ioMode == IOMode::None ? -1 : _ioRank; }
  INLINE_METHOD int    getActiveIOTag() const { return _ioMode == IOMode::None ? -1 : _ioTag; }
#endif // WITH_MPI
};

template <class T>
tarch::logging::Log peano4::stacks::Stack<T>::_log("peano4::stacks::Stack<T>");
