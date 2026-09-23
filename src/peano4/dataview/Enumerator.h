// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"

namespace peano4::dataview {
  template <class Element, int Cardinality>
  class Enumerator;
} // namespace peano4::dataview

/**
 * Provides indexed access to a cell's pooled entities.
 *
 * Negative slots resolve to the container's sentinel.
 */
template <class Element, int Cardinality>
class peano4::dataview::Enumerator final {
public:
  INLINE_METHOD Enumerator(Element* pool, const int* slots, int sentinelSlot);

  INLINE_METHOD Element& operator()(int index) const;

private:
  Element* _elements[Cardinality];
};

#include "peano4/dataview/Enumerator.cpph"
