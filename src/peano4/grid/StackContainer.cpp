// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackContainer.h"

#include "peano4/grid/GridVertex.h"
#include "peano4/stacks/Stack.h"

namespace {
  /**
   * Compiles the container's template bodies with the library. Taking the address of
   * each member is an odr-use, so the compiler emits the body.
   */
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    using VertexContainer = peano4::grid::StackContainer<peano4::stacks::Stack<peano4::grid::GridVertex>>;

    [[maybe_unused]] auto getLocalExchangeStack = &VertexContainer::getLocalExchangeStack;
    [[maybe_unused]] auto getPoolSlot           = &VertexContainer::getPoolSlot;
    [[maybe_unused]] auto getPoolSize           = &VertexContainer::getPoolSize;
    [[maybe_unused]] auto resizePool            = &VertexContainer::resizePool;
    [[maybe_unused]] auto replicatePoolSlots    = &VertexContainer::replicatePoolSlots;
    [[maybe_unused]] auto migratePoolSlots      = &VertexContainer::migratePoolSlots;
    [[maybe_unused]] auto initialiseSerialSlot  = &VertexContainer::initialiseSerialSlot;
    [[maybe_unused]] auto growLocalSlots        = &VertexContainer::growLocalSlots;
    [[maybe_unused]] auto size                  = &VertexContainer::size;
    [[maybe_unused]] auto clear                 = &VertexContainer::clear;
  }
} // namespace
