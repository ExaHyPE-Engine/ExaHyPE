// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "toolbox/loadbalancing/AbstractLoadBalancing.h"
#include "tarch/logging/Log.h"

#include <queue>
#include <initializer_list>



namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      class Hardcoded;
    }
  }
}


/**
 * Hardcoded domain decomposition strategy
 *
 * Hardcoded load balancing is useful if you know exactly which domain
 * decomposition you want to build up at which time, or if you have to debug a
 * particular (deterministic) setup. It also comes in handy whenever you can
 * transform your dynamic load balancing challenge into an offline problem.
 * The loadbalancing's Python API contains all the tools to derive good
 * balancing strategies for offline problems.
 *
 * Please consult the constructor Hardcoded::Hardcoded() for details on how
 * to pipe a certain load balancing setup into the load balancing decision.
 *
 *
 * ## Support for files
 *
 * I originally thought it would be nice to have support for hard-coded
 * load-balancing being encoded in a file. However, this contradicts Peano's
 * philosophy that everything should be in the Python script. Therefore, this
 * strategy does **not** support file I/O but the load balancing's Python
 * interface does.
 *
 *
 * ## Trivia
 *
 * There's also a hardcoded orchestration strategy in tarch::multicore::orchestration::Hardcoded.
 * This is a totally different notion of being hardcoded: There's the
 * decision-making is hardcoded whereas here the actual decisions are. In this
 * sense, the present name might be chosen inappropriately.
 *
 *
 * ## Fit to system configuration
 *
 * The idea behind the hardcoded strategy is that users pipe in a particular
 * configuration tied to their specific choice of ranks. That is, if the rank
 * count changes, they'd likel instantiate the hardcoded strategy with a
 * different setup.
 *
 * If a hardcoded setup does not use all ranks, i.e. if a user increases the
 * MPI rank cound but fails to amend the configuration of the hardcoded
 * scheme, then there's nothing we can do here. Indeed, there might be cases
 * where this makes sense, if the hardcoded strategy is used in combination
 * with another load balancing scheme.
 *
 * If the strategy is used with fewer ranks than the configuration uses, i.e.
 * if the hardcoded strategy wants to deploy to rank 20 and there are only
 * 10 ranks, then we write an error and continue with a mock up. See
 * validateAndAddSplitInstruction() for details.
 */
class toolbox::loadbalancing::strategies::Hardcoded: public toolbox::loadbalancing::AbstractLoadBalancing {
  public:
    /**
     * Construct hard-coded load balancing strategy
     *
     * The splits that are to be triggered are passed here as three initialiser
     * lists. So if you pass
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * {4,5,8}, {2,2,2}, {11,12,13}, {22,23,24}
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * then tree 2 will split in the grid sweeps 4, 5 and 8. It will try to
     * deploy 11, 12 or 13 cells to the ranks 22, 23 or 24, respectively.
     *
     * ## Implementation
     *
     * We do not work with initialiser lists internally. Instead, the
     * constructor takes the list and transforms it into a sequence of
     * Split objects which are then held internally.
     *
     * I originally wanted the constructor to dumb some info on what
     * type of load balancing exactly had been constructed. However, some codes
     * use the load balancing object as static variable, so it might come up
     * before the logging infrastructure is properly configured. This means
     * that the logInfo will crash. So better stay silent and avoid logInfo()
     * calls here.
     *
     * @see Documentation of class Hardcoded for details on any error handling.
     */
    Hardcoded(
      std::initializer_list<int> gridSweeps,
      std::initializer_list<int> splittingTrees,
      std::initializer_list<int> numberOfCells,
      std::initializer_list<int> destinationRanks,
      std::initializer_list<peano4::SplitInstruction::Mode> modes
    );

    /**
     * Shortcut around other constructor
     *
     * In this variant, all split modes are of the same type and there's a
     * default.
     */
    Hardcoded(
      std::initializer_list<int> gridSweeps,
      std::initializer_list<int> splittingTrees,
      std::initializer_list<int> numberOfCells,
      std::initializer_list<int> destinationRanks,
      peano4::SplitInstruction::Mode  mode = peano4::SplitInstruction::Mode::BottomUp
    );

    virtual void finishStep() override;

    virtual void finishSimulation() override;
  private:
    static tarch::logging::Log  _log;

    struct Split {
      int  gridSweep;      // Changed from timeStamp
      int  splittingTree;
      int  numberOfCells;
      int  destinationRank;
      peano4::SplitInstruction::Mode mode;
      Split( const Split& split );
      Split( int gridSweep_, int splittingTree_, int numberOfCells_, int destinationRank_, peano4::SplitInstruction::Mode mode_ );

      std::string toString() const;
    };

    std::queue< Split >   _splits;

    /**
     * Grid sweep represents the iteration number or finishStep() call count.
     * This better reflects that splits occur during specific grid traversals
     * rather than at specific points in time.
     */
    int    _currentGridSweep;

    void validateAndAddSplitInstruction( Split instruction );
};



