// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_LOAD_BALANCING_HARDCODED_
#define _TOOLBOX_LOAD_BALANCING_HARDCODED_



#include "tarch/logging/Log.h"

#include <queue>
#include <initializer_list>



namespace toolbox {
  namespace loadbalancing {
    class Hardcoded;
  }
}


/**
 * Hardcoded domain decomposition strategy
 *
 * Hardcoded lb is useful if you know exactly which domain decomposition you
 * want to build up at which time, or if you have to debug a particular
 * (deterministic) setup.
 *
 */
class toolbox::loadbalancing::Hardcoded {
  public:
    /**
     * @todo I'd like to have support for a file here later on which contains
     * entries alike
     *
<pre>
 10: tree 15 splits into 4 cells to rank 4
 25: tree 16 splits into 5 cells to rank 2
</pre>
     *
     *
     * The splits that are to be triggered are passed here as three initialiser
     * lists. So if you pass
     *
     * <pre> {4,5,8}, {2,2,2}, {11,12,13}, {22,23,24} </pre>
     *
     * then tree 2 will split in the grid sweeps 4, 5 and eight. It will try to
     * deploy 11, 12 or 13 cells to the ranks 22, 23 or 24, respectively.
     */
    Hardcoded(std::initializer_list<int> timeStamps, std::initializer_list<int> splittingTrees, std::initializer_list<int> numberOfCells,  std::initializer_list<int> destinationRanks );

    void finishStep();

    void finishSimulation();

    /**
     * I need the stats here mainly for debugging purposes.
     */
    void dumpStatistics();

    bool hasSplitRecently() const;

  private:
    static tarch::logging::Log  _log;

    struct Split {
      int  timeStamp;
      int  splittingTree;
      int  numberOfCells;
      int  destinationRank;
      Split( const Split& split );
      Split( int  timeStamp_, int  splittingTree_, int  numberOfCells_, int  destinationRank_ );
    };

    std::queue< Split >   _splits;

    /**
     * Time stamp might be the slightly wrong wording. It is actually grid iteration
     * or finishStep() calls.
     */
    int                    _currentTimeStamp;
};


#endif

