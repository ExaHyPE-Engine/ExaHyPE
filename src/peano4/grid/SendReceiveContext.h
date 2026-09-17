// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace peano4::grid {
  /**
   * Identifies the source relationship of two entity copies.
   *
   * - `InterRank`: merge() combines vertex or face copies received from two ranks.
   * - `IntraRank`: merge() combines copies written by two subtree tasks on one rank.
   * - `Consolidation`: applyIncomingCopy() rebuilds a rank-owned slot from task-local
   *   copies before boundary packing.
   *
   * A periodic entity within one rank uses one storage slot. A periodic entity on
   * a rank boundary uses `InterRank`.
   */
  enum class SendReceiveContext { InterRank, IntraRank, Consolidation };

  template <typename Element, typename Marker>
  INLINE_METHOD void applyIncomingCopy(
    Element&                         data,
    peano4::grid::SendReceiveContext context,
    const Marker&                    marker,
    int                              treeId,
    const Element&                   incoming
  );
} // namespace peano4::grid

#include "peano4/grid/SendReceiveContext.cpph"
