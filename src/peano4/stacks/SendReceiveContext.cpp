// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SendReceiveContext.h"

std::string peano4::stacks::toString(const peano4::stacks::SendReceiveContext context) {
  switch (context) {
  case peano4::stacks::SendReceiveContext::InterRank:
    return "InterRank";
  case peano4::stacks::SendReceiveContext::IntraRank:
    return "IntraRank";
  case peano4::stacks::SendReceiveContext::Consolidation:
    return "Consolidation";
  }
  return "<undef>";
}

namespace {
  /** Answers the two calls applyIncomingCopy() makes on the data it applies a copy to. */
  struct MergingElement {
    static constexpr bool RebuildsConsolidationSlotByMerge = true;

    bool receiveAndMerge(
      [[maybe_unused]] const int                                marker,
      [[maybe_unused]] const peano4::stacks::SendReceiveContext context
    ) const {
      return true;
    }
    void merge(
      [[maybe_unused]] const peano4::stacks::SendReceiveContext context,
      [[maybe_unused]] const MergingElement&                    neighbour,
      [[maybe_unused]] const int                                marker,
      [[maybe_unused]] const int                                treeId
    ) {}
  };

  /**
   * Declares no RebuildsConsolidationSlotByMerge, so applyIncomingCopy() assigns
   * on a consolidation.
   */
  struct AssignedElement {
    bool receiveAndMerge(
      [[maybe_unused]] const int                                marker,
      [[maybe_unused]] const peano4::stacks::SendReceiveContext context
    ) const {
      return true;
    }
    void merge(
      [[maybe_unused]] const peano4::stacks::SendReceiveContext context,
      [[maybe_unused]] const AssignedElement&                   neighbour,
      [[maybe_unused]] const int                                marker,
      [[maybe_unused]] const int                                treeId
    ) {}
  };

  /** Returns the incoming copy, which a caller reads out of a pool slot. */
  template <typename Element>
  struct IncomingCopySource {
    Element operator()() const { return Element(); }
  };
} // namespace

template void peano4::stacks::applyIncomingCopy<MergingElement, int, IncomingCopySource<MergingElement>>(
  MergingElement&,
  const peano4::stacks::SendReceiveContext,
  const int&,
  const int,
  const IncomingCopySource<MergingElement>&
);

template void peano4::stacks::applyIncomingCopy<AssignedElement, int, IncomingCopySource<AssignedElement>>(
  AssignedElement&,
  const peano4::stacks::SendReceiveContext,
  const int&,
  const int,
  const IncomingCopySource<AssignedElement>&
);
