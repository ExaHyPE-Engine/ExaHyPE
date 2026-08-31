// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/topology/Tree.h"
#include "peano4/topology/TreeTraversal.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct GridStatistics;
} // namespace peano4::grid

namespace peano4::topology {
  class TopologyManager;
} // namespace peano4::topology

/**
 * Owns the rank's grid topology and runs its sweeps.
 *
 * Every rank derives the same refinement and ownership plans. The rank's tree
 * holds the locally owned regions and one ring of neighbours around them, and
 * the manager exchanges boundary data with the ranks that ring touches.
 */
class peano4::topology::TopologyManager final {
public:
  TopologyManager(const TopologyManager&)            = delete;
  TopologyManager(TopologyManager&&)                 = delete;
  TopologyManager& operator=(const TopologyManager&) = delete;
  TopologyManager& operator=(TopologyManager&&)      = delete;
  ~TopologyManager()                                 = default;

  static peano4::topology::TopologyManager& getInstance();

  void init(
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const std::bitset<DIMENSIONS>&               periodicBC = std::bitset<DIMENSIONS>()
  );

  void shutdown();

  void scheduleRegularGridConstruction(int targetLevel);

  void markPersistentDataLive();

  void enableDynamicLoadBalancing(bool enable);

  void traverse(peano4::grid::GridTraversalObserver& observer);

  bool controlEventsChangePlan(const std::vector<peano4::grid::GridControlEvent>& gridControlEvents);

  const peano4::grid::GridStatistics& getGridStatistics() const;

  peano4::topology::TreeTraversal::SubtreeTaskLoad getSubtreeTaskLoad() const;

  double getAccumulatedTaskSeconds() const;

  const tarch::la::Vector<DIMENSIONS, double>& getDomainOffset() const;
  const tarch::la::Vector<DIMENSIONS, double>& getDomainWidth() const;
  const std::bitset<DIMENSIONS>&               getPeriodicBC() const;

  static int deriveSpawnLevel(int targetLevel, int numberOfRanks);

  /**
   * Selects how a regrid realises its delta.
   *
   * In place means the standing tree becomes the union, so no tree builds and
   * no migration hop runs. A moved partition or grown gaps move the entities
   * onto a rebuilt structure after the transition sweep.
   */
  enum class RegridRealisation {
    EraseInPlace,
    EraseOnRebuiltTree,
    RefineInPlace,
    MixedInPlace,
    MixedWithAppendedUnion
  };

  static RegridRealisation selectRegridRealisation(
    bool pureErase,
    bool pureRefine,
    bool partitionMoved,
    bool gapsStayBounded
  );

  /**
   * Keeps the standing partition while its largest rank load stays within
   * this percentage above the largest load of the balanced partition, so a
   * regrid that shifts few leaves realises in place and moves no block.
   */
  static constexpr int RepartitionTolerancePercent = 2;

private:
  TopologyManager() = default;

  static peano4::topology::TopologyManager _singleton;

  int _targetLevel{-1};

  tarch::la::Vector<DIMENSIONS, double> _domainOffset{};
  tarch::la::Vector<DIMENSIONS, double> _domainWidth{};
  std::bitset<DIMENSIONS>               _periodicBC{};

  std::unique_ptr<peano4::topology::Tree>          _tree{};
  std::unique_ptr<peano4::topology::TreeTraversal> _traversal{};

  /** Holds the new structure prepared before the migration hop; a regrid without a hop leaves it empty. */
  std::unique_ptr<peano4::topology::Tree> _preparedNewTree{};
  /** Holds the plan-query view of an in-place regrid's new plan until the tree adopts its tables. */
  std::unique_ptr<peano4::topology::Tree>          _transitionPlanView{};
  std::unique_ptr<peano4::topology::TreeTraversal> _preparedNewTraversal{};

  /** Keeps the structures a migration hop replaced, so a rebuild reuses their buffers. */
  std::vector<std::unique_ptr<peano4::topology::Tree>>          _spareTrees{};
  std::vector<std::unique_ptr<peano4::topology::TreeTraversal>> _spareTraversals{};

  peano4::topology::Tree::RefinementPlan _refinementPlan{};

  /** Holds the packed recursive ownership plan consumed by Tree::buildRegularGrid(). */
  std::vector<int> _blockOwners{};

  /** Stores the balanced ownership plan adopted by the final migration hop. */
  std::vector<int> _pendingBlockOwners{};

  /**
   * Caches the control events of the last plan evaluation with its verdict
   * and the plan it produced, so a repeated query and the sweep that realises
   * a change evaluate the set once. A realised change drops the cache.
   */
  std::vector<peano4::grid::GridControlEvent> _lastEvaluatedControlEvents{};
  peano4::topology::Tree::RefinementPlan      _lastEvaluatedPlanUpdate{};
  bool                                        _lastEvaluationChangedPlan{false};

  bool _persistentDataLive{false};

  /** Lets a regrid move a block to another rank. */
  bool _dynamicLoadBalancing{true};

  /** Stops the per-sweep vote after every rank has adopted the live-data flag. */
  bool _persistentDataLiveUniform{false};

  std::optional<peano4::topology::TreeTraversal::RegridTransition> _regridTransition{};

  bool _transitionSweepPending{false};

  /**
   * The pending transition traverses the standing structure in place, so the
   * interface entities' current copies flush into the entity slots the
   * transition reads before that sweep starts.
   */
  bool _transitionFlushPending{false};

  bool _transitionOnNewStructure{false};

  bool _transitionRestoresDecompositionSnapshot{false};

  bool _transitionPrunesStructure{false};

  peano4::topology::Tree::RefinementPlan _transitionPruneFlags{};

  /** Skips a boundary finish after migration has already drained the standing exchange. */
  bool _skipStandingBoundaryFinish{false};

  /**
   * Accumulates task time across all sweeps since the run began, in seconds.
   * Two samples around a step give the task time of that step.
   */
  double _accumulatedTaskSeconds{0.0};

  /** Keeps one rank-level observer clone per type across sweeps. */
  std::map<std::type_index, std::unique_ptr<peano4::grid::GridTraversalObserver>> _rankObserverOfType{};

  int deriveSpawnLevel() const;

  peano4::grid::GridTraversalObserver& rankObserverClone(peano4::grid::GridTraversalObserver& observer);

  std::unique_ptr<peano4::topology::Tree>          acquireTree();
  std::unique_ptr<peano4::topology::TreeTraversal> acquireTraversal();

  void buildTree(bool resizePools, const peano4::topology::Tree::RefinementPlan& refinementPlan);

  bool derivePlanUpdate(
    const std::vector<peano4::grid::GridControlEvent>& gridControlEvents,
    peano4::topology::Tree::RefinementPlan&            updatedPlan
  );

  void runSweep(peano4::grid::GridTraversalObserver& observer);
};
