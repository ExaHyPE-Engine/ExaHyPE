// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/parallel/Node.h"
#include "tarch/logging/Log.h"
#include "toolbox/loadbalancing/AbstractLoadBalancing.h"
#include "toolbox/loadbalancing/metrics/CellCount.h"

#include <map>

namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      class RecursiveSplit;
    } // namespace strategies
  }   // namespace loadbalancing
} // namespace toolbox

/**
 * Initially begin with 1 tree.
 * Then when the load balancing strategy is called, sort all the tress in
 * reverse order of the number of cells. Examine the tree with the largest
 * number of cells. If there are enough available cores and the number of cells
 * exceeds a threshold, then split this tree. The number of trees created
 * from splitting this tree i.e. the recursiveFactor is calculated as the ratio
 * of the number of the cells of the tree compared to the average number of
 * cells in a tree. This process is then repeated for all remaining trees which
 * have more cells than the average number of cells in a tree.
 *
 * Bottom-up splitting is the mode of splitting.
 */
class toolbox::loadbalancing::strategies::RecursiveSplit: public toolbox::loadbalancing::AbstractLoadBalancing {
public:
  RecursiveSplit(
    Configuration* configuration = new DefaultConfiguration(),
    CostMetrics*   costMetrics   = new toolbox::loadbalancing::metrics::CellCount()
  );
  virtual ~RecursiveSplit() override = default;

  virtual void finishStep() override;

  struct SpacetreeComparator;

private:
  static tarch::logging::Log _log;

  int _previousNumberOfCells;
  int _numberOfStableGridIterations;
  int _numTrees;

  bool _hasRecentlySplit;
  int  _traversalsSinceSplit;

  /**
   * Return 0 if the load balancing should not split (yet) and otherwise
   * return number of splits required.
   */
  int getNumberOfRequiredSplits(int spacetreeId) const;

  void updateLoadBalancing();

  void triggerSplit(int sourceTree, int numberOfCells, int targetRank);
};
