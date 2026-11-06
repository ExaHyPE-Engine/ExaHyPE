// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/CellConnectivity.h"

namespace peano4 {
  struct Prolongation;
  struct Restriction;
  struct TransferredCell;
  struct RankTransfer;
  struct Regrid;
} // namespace peano4

/**
 * Fills the new slot of one child from the old slot of its parent.
 */
struct peano4::Prolongation final {
  int parentSlot{-1};
  int childSlot{-1};
  int childPosition{0};
};

/**
 * Fills the new slot of one parent from the old slots of its children in linearised order.
 */
struct peano4::Restriction final {
  int parentSlot{-1};
  int childSlots[peano4::ChildrenPerCell]{};
};

/**
 * The old slots a rank reads one sent cell from: The cell's own slot, its parent's slot or its
 * children's slots. Exactly one of the three is valid.
 */
struct peano4::TransferredCell final {
  int keptSlot{-1};
  int parentSlot{-1};
  int childPosition{0};
  int childSlots[peano4::ChildrenPerCell]{};
};

/**
 * The cells this rank sends to a partner rank and the slots it receives from that rank. Both lists
 * are sorted by level and lattice position. Both ranks therefore derive the same order.
 */
struct peano4::RankTransfer final {
  int                          rank{-1};
  std::vector<TransferredCell> sentCells{};
  std::vector<int>             receivedSlots{};
};

/**
 * The slot moves of one regrid. A prolongation, a restriction or a receive fills every new slot
 * outside the slots of the kept cells. A new slot never reuses a slot freed in the same regrid.
 */
struct peano4::Regrid final {
  int                       oldNumberOfSlots{0};
  int                       newNumberOfSlots{0};
  std::vector<Prolongation> prolongations{};
  std::vector<Restriction>  restrictions{};
  std::vector<RankTransfer> rankTransfers{};
  std::vector<int>          freedSlots{};
  std::vector<int>          createdSlots{};
};
