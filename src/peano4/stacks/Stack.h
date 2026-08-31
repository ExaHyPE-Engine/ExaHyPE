// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/IOMode.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/PersistentChannel.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

namespace peano4::stacks {
  template <class T>
  class Stack;
} // namespace peano4::stacks

template <class T>
class peano4::stacks::Stack {
public:
  static_assert(std::is_move_constructible_v<T>);

  /** A contiguous range of elements handed out by pushBlock() and popBlock(). */
  class View final {
  public:
    INLINE_METHOD std::size_t size() const;

    INLINE_METHOD T&       get(const std::size_t index);
    INLINE_METHOD const T& get(const std::size_t index) const;

    INLINE_METHOD void set(const std::size_t index, T&& value);

    template <class... Args>
    INLINE_METHOD void emplace(const std::size_t index, Args&&... args) noexcept(
      std::is_nothrow_destructible_v<T> and std::is_nothrow_constructible_v<T, Args...>
    );

    INLINE_METHOD void emplaceDefault(
      const std::size_t index
    ) noexcept(std::is_nothrow_destructible_v<T> and std::is_nothrow_default_constructible_v<T>);

  private:
    friend class Stack<T>;

    const std::size_t _size;
    const std::size_t _baseElement;
    Stack<T>*         _stack;

    INLINE_METHOD View(const std::size_t numberOfElements, const std::size_t baseElement, Stack<T>* const owner);
  };

  INLINE_METHOD Stack() noexcept = default;
  Stack(const Stack&)            = delete;
  INLINE_METHOD        Stack(Stack&& other) noexcept;
  Stack&               operator=(const Stack&) = delete;
  INLINE_METHOD Stack& operator=(Stack&& other) noexcept;
  virtual ~Stack() noexcept = default;

  INLINE_METHOD std::size_t push(const T& element)
    requires(std::is_copy_constructible_v<T> and (std::is_copy_assignable_v<T> or std::is_move_assignable_v<T>));

  INLINE_METHOD std::size_t push(T&& element);

  template <typename... Args>
  INLINE_METHOD std::size_t emplace(Args&&... args);

  INLINE_METHOD void erase(const std::size_t index);

  INLINE_METHOD void reserve(const std::size_t size);
  INLINE_METHOD void resize(const std::size_t size);

  INLINE_METHOD T&       operator[](const std::size_t index);
  INLINE_METHOD const T& operator[](const std::size_t index) const;

  INLINE_METHOD T pop();

  INLINE_METHOD T&       top(const std::size_t shift = 0);
  INLINE_METHOD const T& top(const std::size_t shift = 0) const;

  INLINE_METHOD View popBlock(const std::size_t numberOfElements);
  INLINE_METHOD View pushBlock(const std::size_t numberOfElements);

  INLINE_METHOD void clone(const Stack<T>& other)
    requires std::is_copy_constructible_v<T>;

  INLINE_METHOD std::string toString() const;

  INLINE_METHOD std::size_t size() const noexcept;
  INLINE_METHOD bool        empty() const noexcept;

  INLINE_METHOD void clear();
  INLINE_METHOD void shallowClear();

  virtual void startSend(
    const peano4::stacks::SendReceiveContext context,
    const int                                rank,
    const int                                tag,
    const MPI_Comm                           comm
  );

  virtual void startReceive(
    const peano4::stacks::SendReceiveContext context,
    const int                                rank,
    const int                                tag,
    const MPI_Comm                           comm,
    const int                                numberOfElements
  );

  virtual bool tryToFinishSendOrReceive();

  INLINE_METHOD peano4::stacks::IOMode getIOMode() const;

  INLINE_METHOD int getActiveIORank() const;
  INLINE_METHOD int getActiveIOTag() const;

protected:
  INLINE_METHOD static int getMPIByteCount(const std::size_t numberOfElements, const std::size_t bytesPerElement);

  std::vector<T> _data{};
  std::size_t    _logicalSize{0};

  peano4::stacks::IOMode _transferMode{peano4::stacks::IOMode::None};
  int                    _transferRank{-1};
  int                    _transferTag{-1};

  /** Reuses an unchanged transfer through MPI_Start(). */
  tarch::mpi::PersistentChannel _persistentTransfer{};
  bool                          _transferIsActive{false};
};

#include "peano4/stacks/Stack.cpph"
