// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace tarch {
  template <typename Element>
  struct Span;
} // namespace tarch

/** Addresses the size consecutive elements that start at data. */
template <typename Element>
struct tarch::Span final {
  Element* data{nullptr};
  int      size{0};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD Element& operator[](int index) const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD Element* begin() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD Element* end() const noexcept;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool isEmpty() const noexcept;
};

#include "tarch/Span.cpph"
