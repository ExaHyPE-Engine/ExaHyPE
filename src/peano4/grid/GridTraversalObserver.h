// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  class GridTraversalObserver;
  struct GridControlEvent;
  struct GridTraversalEvent;
} // namespace peano4::grid

namespace peano4::datamanagement {
  struct CellMarker;
  struct FaceMarker;
  struct VertexMarker;
} // namespace peano4::datamanagement

/**
 * Connects the rank traversal and its subtree tasks to application data.
 *
 * clone() creates an isolated observer for the rank traversal and for every
 * subtree task. These observers may run concurrently.
 */
class peano4::grid::GridTraversalObserver {
public:
  GridTraversalObserver()                                        = default;
  GridTraversalObserver(const GridTraversalObserver&)            = default;
  GridTraversalObserver(GridTraversalObserver&&)                 = default;
  GridTraversalObserver& operator=(const GridTraversalObserver&) = default;
  GridTraversalObserver& operator=(GridTraversalObserver&&)      = default;
  virtual ~GridTraversalObserver()                               = default;

  /** Sentinel for an entity outside the local computational domain. */
  static constexpr int NoData = -1;

  /**
   * Sentinel for an entity created on entry or destroyed on exit.
   * Hanging status remains a separate marker property.
   */
  static constexpr int CreateOrDestroyGridEntity = -2;

  enum class NodeData { Vertex, Face, Cell };

  virtual void loadNodeData(const GridTraversalEvent& event) {};

  virtual void enterNode(const GridTraversalEvent& event) = 0;
  virtual void leaveNode(const GridTraversalEvent& event) = 0;

  virtual void storeNodeData(const GridTraversalEvent& event) {};

  /**
   * Creates an observer for one rank traversal or subtree task.
   *
   * Calls may overlap. The caller owns the returned observer.
   */
  virtual GridTraversalObserver* clone(int treeId) = 0;

  /**
   * Returns the dynamic-AMR cell requests for the next traversal.
   * Sequence order carries no precedence.
   */
  virtual std::vector<GridControlEvent> getGridControlEvents() const = 0;

  /**
   * Starts one rank traversal or subtree task.
   *
   * Each observer provides the synthetic parent data required by its first
   * enter event.
   *
   * @param x Root-cell centre.
   * @param h Root-cell size.
   */
  virtual void beginTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) = 0;

  /**
   * @see beginTraversal()
   */
  virtual void endTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) = 0;

  /**
   * Merges one partner copy of an interface entity into the subtree task's
   * live pool slot. Cell transfers ignore position.
   */
  virtual void receiveNodeData(
    NodeData                   nodeData,
    int                        position,
    int                        fromStack,
    stacks::SendReceiveContext context,
    const GridTraversalEvent&  event
  ) {};
};
