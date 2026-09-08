// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RegisteredStackContainer.h"

#include "peano4/grid/GridVertex.h"

namespace {
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    using RegisteredContainer = peano4::data::RegisteredStackContainer<peano4::stacks::Stack<peano4::grid::GridVertex>>;

    [[maybe_unused]] const auto containerId     = &RegisteredContainer::getManagedContainerId;
    [[maybe_unused]] const auto taskReplication = &RegisteredContainer::supportsSubtreeTaskReplication;
    [[maybe_unused]] const auto serialSlot      = &RegisteredContainer::initialiseSerialSlot;
    [[maybe_unused]] const auto resizePool      = &RegisteredContainer::resizePoolForTree;
    [[maybe_unused]] const auto migratePool     = &RegisteredContainer::migratePoolForTree;
    [[maybe_unused]] const auto replicate       = &RegisteredContainer::replicateInterfaceSlots;
    [[maybe_unused]] const auto packBoundary    = &RegisteredContainer::packBoundarySlots;
    [[maybe_unused]] const auto transferPool    = &RegisteredContainer::transferPoolSlotsForTree;
    [[maybe_unused]] const auto finish          = &RegisteredContainer::finishBoundaryExchange;
    [[maybe_unused]] const auto localSlots      = &RegisteredContainer::prepareLocalSlots;
    [[maybe_unused]] const auto clear           = &RegisteredContainer::clear;
  }
} // namespace
