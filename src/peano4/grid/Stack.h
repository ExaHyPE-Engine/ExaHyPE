// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"

namespace peano4::grid {
  template <class T>
  class Stack;
} // namespace peano4::grid

/**
 * Stores grid-entity records in a reusable contiguous buffer.
 *
 * Slots above the logical size stay constructed and retain their values until
 * overwritten. Element assignment therefore replaces the complete state.
 */
template <class T>
class peano4::grid::Stack final {
  static_assert(std::is_move_constructible_v<T>, "A stack element has to be movable into and out of the buffer.");

public:
  //! Names the stored record type.
  using ElementType = T;

  /**
   * Represents a contiguous range of stack slots.
   */
  class View final {
  public:
    INLINE_METHOD T&       get(std::size_t index);
    INLINE_METHOD const T& get(std::size_t index) const;

  private:
    friend class Stack<T>;

    INLINE_METHOD View(std::size_t size, std::size_t baseElement, Stack<T>* stack);

    const std::size_t _size;
    const std::size_t _baseElement;
    Stack<T>* const   _stack;
  };

  INLINE_METHOD Stack() noexcept = default;
  Stack(const Stack& other)      = delete;
  INLINE_METHOD        Stack(Stack&& other) noexcept;
  Stack&               operator=(const Stack& other) = delete;
  INLINE_METHOD Stack& operator=(Stack&& other) noexcept;
  ~Stack() noexcept = default;

  INLINE_METHOD std::size_t push(const T& element)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>);
  INLINE_METHOD std::size_t push(T&& element);
  INLINE_METHOD T           pop();

  INLINE_METHOD T&       top(std::size_t shift = 0);
  INLINE_METHOD const T& top(std::size_t shift = 0) const;

  INLINE_METHOD T&       operator[](std::size_t index);
  INLINE_METHOD const T& operator[](std::size_t index) const;

  INLINE_METHOD View pushBlock(std::size_t numberOfElements);

  INLINE_METHOD T*       data() noexcept;
  INLINE_METHOD const T* data() const noexcept;

  INLINE_METHOD std::size_t size() const noexcept;
  INLINE_METHOD bool        empty() const noexcept;

  INLINE_METHOD void resize(std::size_t numberOfElements);
  INLINE_METHOD void clear();
  INLINE_METHOD void shallowClear();

private:
  std::vector<T> _data{};

  //! Counts the slots in use and never exceeds the buffer size.
  std::size_t _logicalSize{0};
};

#include "peano4/grid/Stack.cpph"
