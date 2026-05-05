// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Log.h"

#include <string>
#include <unordered_map>

namespace toolbox::loadbalancing {
  class Blacklist;
} // namespace toolbox::loadbalancing

/**
 * Blacklisting of spacetrees after splits.
 *
 * Rationale:
 * After a split is triggered, the affected tree's local statistics are temporarily
 * skewed (ownership / data movement / reduction latency). If the load balancer
 * immediately considers the same tree again, it may repeatedly pick it as split
 * candidate.
 *
 * We therefore track split candidates in a map and keep them "blacklisted" for a
 * small number of iterations (default: 3). Additionally, if the tree's local cell
 * count has not changed since the split, we keep it on the blacklist to avoid
 * repeatedly triggering splits that don't actually fork off cells.
 *
 * The blacklist entries are kept even when inactive so we can keep cumulative
 * counters (e.g. how often a tree split).
 */
class toolbox::loadbalancing::Blacklist {
public:
  Blacklist()  = default;
  ~Blacklist() = default;

  /**
   * Update blacklist at end of traversal.
   *
   * - If a tree reports that it removed an empty subtree, we extend (or re-enable)
   *   the blacklist to avoid immediately forking further.
   * - If a tree is active on the blacklist but its local cell count did not change
   *   since the baseline snapshot, we keep it blacklisted.
   * - Otherwise, we count down its remaining lifetime until it becomes inactive.
   */
  void update();

  /**
   * Inform blacklist that load balancing has just triggered a split on treeId.
   */
  void notifySplit(int treeId);

  std::string toString() const;

  bool isBlacklisted(int treeId) const;

private:
  static constexpr int InitialLifetime = 3;

  struct Entry {
    /**
     * Remaining iterations to keep the entry active.
     * 0 => inactive (tree is not blacklisted).
     */
    int lifetime = 0;

    /**
     * Baseline snapshot of numberOfLocalUnrefinedCells when the split (or
     * degeneration handling) occurred.
     */
    int baselineNumberOfUnrefinedCells = 0;

    /** Cumulative number of split notifications for this tree. */
    int numberOfSplits = 0;

    /** Cumulative number of times we observed "removed empty subtree". */
    int numberOfDegeneratedChildren = 0;

    Entry() = default;
    explicit Entry(int treeId);
  };


  /**
   * Map treeId -> entry. Presence in the map does not imply active blacklisting;
   * check Entry::lifetime > 0.
   */
  std::unordered_map<int, Entry> _blacklist;
};
