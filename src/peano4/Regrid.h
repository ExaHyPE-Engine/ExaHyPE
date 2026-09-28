// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"

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
 * Names the origin of one cell a rank sends: The old slot of the cell itself, the old slot of
 * its parent with the child's position, or the old slots of its children. Exactly one of the
 * three holds a valid slot.
 */
struct peano4::TransferredCell final {
  int keptSlot{-1};
  int parentSlot{-1};
  int childPosition{0};
  int childSlots[peano4::ChildrenPerCell]{};
};

/**
 * Lists what one rank pair exchanges. The sent cells and the received slots stand in the
 * canonical order of level and lattice position, which both ranks derive alone.
 */
struct peano4::RankTransfer final {
  int                          rank{-1};
  std::vector<TransferredCell> sentCells{};
  std::vector<int>             receivedSlots{};
};

/**
 * The slot moves of one regrid: A kept slot has no entry, every other slot of the new partition
 * is filled by one prolongation, one restriction or one receive, and the old slots of the
 * destroyed cells are freed. No new slot reuses a slot freed in the same regrid. The created
 * slots are the slots of the children of every refined cell and of the parent of every set of
 * coarsened siblings.
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
