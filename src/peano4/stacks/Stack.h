// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/IOMode.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "tarch/Assertions.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/PersistentChannel.h"
#include "tarch/mpi/Request.h"

namespace peano4::stacks {
  template <class T>
  class Stack;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::Stack {
private:
  // static_assert(std::is_standard_layout_v<T>);
  // static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::is_move_constructible_v<T>);

protected:
  std::vector<T> _data{};
  std::size_t    _logicalSize = 0; // Number of logically used elements (<= _data.size())

  IOMode _ioMode = IOMode::None;
  int    _ioRank = -1;
  int    _ioTag  = -1;

  // Restarts an unchanged transfer instead of re-creating it, so a settled
  // boundary exchange pays one MPI_Start per sweep.
  tarch::mpi::PersistentChannel _ioChannel{};
  bool                          _ioMPIActive = false;

  class View {
  private:
    friend class Stack<T>;
    const std::size_t _size;
    const std::size_t _baseElement;
    Stack<T>*         _stack;

    INLINE_METHOD View(std::size_t size, std::size_t base, Stack<T>* stack) :
      _size(size),
      _baseElement(base),
      _stack(stack) {}

  public:
    INLINE_METHOD std::size_t size() const { return _size; }

    INLINE_METHOD T& get(std::size_t index) {
      assertion(index < _size, index, _size);
      return _stack->_data[_baseElement + index];
    }

    INLINE_METHOD const T& get(std::size_t index) const {
      assertion(index < _size, index, _size);
      return _stack->_data[_baseElement + index];
    }

    INLINE_METHOD void set(std::size_t index, T&& value) {
      assertion(index < _size, index, _size);
      _stack->_data[_baseElement + index] = std::move(value);
    }

    template <class... Args>
    INLINE_METHOD void emplace(std::size_t index, Args&&... args) noexcept(
      std::is_nothrow_destructible_v<T> and std::is_nothrow_constructible_v<T, Args...>
    ) {
      assertion(index < _size, index, _size);
      T* slot = &_stack->_data[_baseElement + index];
      std::destroy_at(slot);
      std::construct_at(slot, std::forward<Args>(args)...);
    }

    INLINE_METHOD void emplace_default(
      std::size_t index
    ) noexcept(std::is_nothrow_destructible_v<T> and std::is_nothrow_default_constructible_v<T>) {
      emplace(index);
    }
  };

public:
  INLINE_METHOD Stack() noexcept = default;
  virtual ~Stack() noexcept      = default;
  Stack(const Stack&)            = delete;
  Stack& operator=(const Stack&) = delete;

  INLINE_METHOD Stack(Stack&& other) noexcept :
    _data(std::move(other._data)),
    _logicalSize(other._logicalSize),
    _ioMode(other._ioMode),
    _ioRank(other._ioRank),
    _ioTag(other._ioTag),
    _ioChannel(std::move(other._ioChannel)),
    _ioMPIActive(other._ioMPIActive) {
    // Reset the source to a safe empty state
    other._logicalSize = 0;
    other._ioMode      = IOMode::None;
    other._ioMPIActive = false;
  }

  INLINE_METHOD Stack& operator=(Stack&& other) noexcept {
    if (this != &other) {
      _data        = std::move(other._data);
      _logicalSize = other._logicalSize;
      _ioMode      = other._ioMode;
      _ioRank      = other._ioRank;
      _ioTag       = other._ioTag;
      _ioChannel   = std::move(other._ioChannel);
      _ioMPIActive = other._ioMPIActive;

      other._ioMPIActive = false;
      other._logicalSize = 0;
      other._ioMode      = IOMode::None;
    }
    return *this;
  }

  INLINE_METHOD std::size_t push(const T& element)
    requires(std::is_copy_constructible_v<T> and (std::is_copy_assignable_v<T> or std::is_move_assignable_v<T>))
  {
    if (_logicalSize < _data.size()) {
      // Reuse preserves the physical object and replaces it by assignment.
      if constexpr (std::is_copy_assignable_v<T>) {
        _data[_logicalSize] = element;
      } else {
        _data[_logicalSize] = T(element);
      }
    } else {
      _data.push_back(element);
    }
    return _logicalSize++;
  }

  INLINE_METHOD std::size_t push(T&& element) {
    if (_logicalSize < _data.size()) {
      // Reuse preserves the physical object and replaces it by assignment.
      // T::operator= has to replace the full semantic state, including
      // empty/invalid handles.
      _data[_logicalSize] = std::move(element);
    } else {
      _data.push_back(std::move(element));
    }
    return _logicalSize++;
  }

  template <typename... Args>
  INLINE_METHOD std::size_t emplace(Args&&... args) {
    if (_logicalSize < _data.size()) {
      _data[_logicalSize] = T(std::forward<Args>(args)...);
    } else {
      _data.emplace_back(std::forward<Args>(args)...);
    }
    return _logicalSize++;
  }

  /// The removed element is replaced with the last element, so the order is
  /// not preserved.
  INLINE_METHOD void erase(std::size_t index) {
    assertion(index < _logicalSize);
    _data[index] = std::move(_data[_logicalSize - 1]);
    --_logicalSize;
  }

  INLINE_METHOD void reserve(const std::size_t size) { _data.reserve(size); }

  INLINE_METHOD void resize(const std::size_t size) {
    if (size <= _data.size()) {
      _logicalSize = size;
    } else {
      _data.reserve(size);
      while (_data.size() < size) {
        _data.emplace_back();
      }
      _logicalSize = size;
    }
  }

  INLINE_METHOD T& operator[](std::size_t index) {
    assertion(index < _logicalSize);
    return _data[index];
  }

  INLINE_METHOD const T& operator[](std::size_t index) const {
    assertion(index < _logicalSize);
    return _data[index];
  }

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
      _data.reserve(_logicalSize);
      while (_data.size() < _logicalSize) {
        _data.emplace_back();
      }
    }
    // Existing physical slots are reused as-is. Callers that fill the view
    // must assign values whose assignment operators fully replace semantic
    // state, including empty/invalid handles.
    return View(numberOfElements, base, this);
  }

  INLINE_METHOD void clone(const Stack<T>& other)
    requires std::is_copy_constructible_v<T>
  {
    assertion(this->empty());
    const std::size_t n = other._logicalSize;
    _data.clear();
    _data.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      _data.emplace_back(other._data[i]);
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

  INLINE_METHOD void shallowClear() {
    // Keep constructed elements for reuse; their old semantic state remains
    // present until a later push()/View::set() assignment replaces it.
    _logicalSize = 0;
  }

  /**
   * Posts a non-blocking send of the whole container. It pairs up with a later
   * tryToFinishSendOrReceive(): the message reads from the container's buffer
   * until that call reports completion, so the container must not be modified
   * while the send is active.
   *
   * @param context Exchange the data belongs to. All contexts ship the same
   *   payload, so the context does not select how the data is described.
   * @param rank Destination rank.
   * @param tag MPI tag.
   * @param comm MPI communicator.
   */
  void
  startSend([[maybe_unused]] const SendReceiveContext context, const int rank, const int tag, const MPI_Comm comm) {
    assertion(_ioMode == IOMode::None);
    assertion(size() > 0);
    assertion(not _ioMPIActive);

    _ioMode = IOMode::MPISend;
    _ioTag  = tag;
    _ioRank = rank;

    logDebug(toString());

    // MPI counts in an int, so counting bytes rather than elements lowers the
    // number of entries one message can carry by sizeof(T).
    const std::size_t numberOfBytes = size() * sizeof(T);
    assertion(
      numberOfBytes <= static_cast<std::size_t>(std::numeric_limits<int>::max()),
      numberOfBytes,
      size(),
      sizeof(T)
    );

    _ioChannel.startSend(_data.data(), static_cast<int>(numberOfBytes), _ioRank, _ioTag, comm);
    _ioMPIActive = true;
  }

  /**
   * Initiates a non-blocking receive into the container. @see startSend()
   *
   * @param context Exchange the data belongs to. All contexts ship the same
   *   payload, so the context does not select how the data is described.
   * @param rank Source rank.
   * @param tag MPI tag.
   * @param comm MPI communicator.
   * @param numberOfElements Number of elements expected.
   */
  void startReceive(
    [[maybe_unused]] const SendReceiveContext context,
    const int                                 rank,
    const int                                 tag,
    const MPI_Comm                            comm,
    const int                                 numberOfElements
  ) {
    assertion(_ioMode == IOMode::None, rank, tag, numberOfElements);
    assertion(numberOfElements > 0, rank, tag, numberOfElements);
    assertion(not _ioMPIActive);

    _ioMode = IOMode::MPIReceive;
    _ioTag  = tag;
    _ioRank = rank;

    logDebug(toString());

    resize(numberOfElements);

    // MPI counts in an int, so counting bytes rather than elements lowers the
    // number of entries one message can carry by sizeof(T).
    const std::size_t numberOfBytes = static_cast<std::size_t>(numberOfElements) * sizeof(T);
    assertion(
      numberOfBytes <= static_cast<std::size_t>(std::numeric_limits<int>::max()),
      numberOfBytes,
      numberOfElements,
      sizeof(T)
    );

    _ioChannel.startReceive(_data.data(), static_cast<int>(numberOfBytes), _ioRank, _ioTag, comm);
    _ioMPIActive = true;
  }

  /**
   * One completion test for the active transfer. A stack without an active
   * transfer reports completion straight away. A completed send clears the
   * stack, as the data now travels through MPI; a completed receive keeps the
   * freshly arrived payload. Afterwards the state is IOMode::None.
   *
   * @return Whether no transfer is active any more.
   */
  bool tryToFinishSendOrReceive() {
    if (_ioMode == IOMode::None or not _ioMPIActive) {
      return true;
    }

    int flag = 0;
    tarch::mpi::test(_ioChannel.getRequest(), &flag, MPI_STATUS_IGNORE);
    if (flag) {
      if (_ioMode == IOMode::MPISend) {
        shallowClear();
        logDebug("Send complete: " << toString());
      } else if (_ioMode == IOMode::MPIReceive) {
        logDebug("Receive complete: " << toString());
      }
      _ioMode      = IOMode::None;
      _ioMPIActive = false;

      return true;
    }

    return false;
  }

  INLINE_METHOD IOMode getIOMode() const { return _ioMode; }

  /**
   * Partner rank and tag of the active transfer; both are -1 while none is
   * active.
   */
  INLINE_METHOD int getActiveIORank() const { return _ioMode == IOMode::None ? -1 : _ioRank; }
  INLINE_METHOD int getActiveIOTag() const { return _ioMode == IOMode::None ? -1 : _ioTag; }
};
