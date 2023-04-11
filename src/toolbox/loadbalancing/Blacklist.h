// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <string>
#include <map>


#include "tarch/logging/Log.h"


namespace toolbox {
  namespace loadbalancing {
    class Blacklist;
  }
}



/**
 * Blacklisting
 *
 * The recursive subdivision works with one tree at a time: It always identifies the
 * one tree with the biggest load and subdivides this one further. A tree however
 * requires two traversals to split. If a tree is identified as split candidate, the
 * code hence will tell it to split, but will try to split exactly the same tree again
 * in the subsequent iteration.
 *
 * The tree's stats (how many cells does it own) at this point are skewed, as the
 * tree has not yet successfully given away cells. Indeed, a tree needs at least three
 * iterations to 'recover'. The first one initialises the split. The second traversal
 * conducts the split and eventually moves around data, but the ownership in this
 * traversal still is with the master. Therefore, the master still has all the local
 * cells. After the third split, the master has actually given away its cells.
 *
 * Therefore, we work with a blacklist map: Whenever we have identified a tree that is to
 * be split, we memorise it and give it an internal weight of 3. Memorise means it is
 * stored in a map. As long as a rank is on the blacklist map, it may not be selected
 * as potential split tree. After each iteration, we reduce the weight of the blacklisted
 * trees by one. If a weight becomes zero, we eventually remove it from the blacklist.
 *
 *
 */
class toolbox::loadbalancing::Blacklist {
  public:
    Blacklist();

    /**
     * Update blacklist at end of traversal
     *
     * - If a tree has unsuccessfully tried to merge its children, then we
     *   increase its blacklist lifetime, so it at least does not start to
     *   fork further.
     * - If a tree is on the blacklist but has not yet changed its number
     *   of local cells, it means that this tree has forked before (so was
     *   added to the blacklist) but then was not able to split some cells
     *   off. So it should remain on the blacklist to ensure that we are not
     *   trying to split this tree over and over again.
     */
    void update();

    /**
     * Inform blacklist that the load balancing has just triggered a split.
     * newParent is the number of the tree which has triggered this split.
     */
    void triggeredSplit( int newParent );

    std::string toString() const;

    bool isBlacklisted( int treeNumber ) const;
  private:
    struct BlacklistData {
      /**
       * Remaining steps how long this blacklist entry should stay on. If the
       * value equals zero, the blacklist entry is inactive, i.e. the
       * underlying tree is not on the blacklist anymore. We usually keep the
       * entry in our data structure however, so we can remember information
       * such as the total number of splits per tree.
       */
      int lifetime;

      /**
       * Store number of unrefined cells at time of split
       */
      int numberOfUnrefinedCells;

      int numberOfSplits;
      int numberOfDegeneratedChildren;

      /**
       * New blacklist entry for entry which has not yet been on blacklist
       * before, i.e. which has to be unknown to us beforehand.
       *
       * @param treeNumber Number of tree that is to be blacklisted. We do
       *   not store this number, but we use the number to query the tree
       *   on its number of unrefined octants.
       */
      BlacklistData(int treeNumber);
    };

    static tarch::logging::Log _log;

    /**
     * Map of trees numbers onto blacklist entries. Being in that map does
     * not necessarily mean a tree is blacklisted. For this, we have to study
     * its lifetime counter.
     *
     * @see BlacklistData
     */
    std::map< int, BlacklistData>    _blacklist;

};


