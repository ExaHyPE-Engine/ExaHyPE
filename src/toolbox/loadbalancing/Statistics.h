// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/mpi/mpi.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"


#include "State.h"


namespace toolbox {
  namespace loadbalancing {
    class Statistics;
  }
}



/**
 * Statistics helper routine for load balancing
 *
 * Please note that we assume a SPMD paradigm, i.e. if one rank uses
 * statistics, the other ranks all have to use such an object, too. The
 * individual objects communicate with each other.
 *
 */
class toolbox::loadbalancing::Statistics {
  public:
    /**
     * Set all the MPI requests to nullptr.
     */
    Statistics();

    std::string toString() const;

    /**
     * Typically called by
     *
     * - finishSimulation() and
     * - finishStep()
     *
     * but we also call it from updateGlobalView() before we issue new
     * collective MPI calls.
     */
    void waitForGlobalDataExchange();

    void updateGlobalView();

    int getLocalNumberOfInnerUnrefinedCells() const;
    int getGlobalNumberOfInnerUnrefinedCells() const;
    int getGlobalNumberOfRanksWithEnabledLoadBalancing() const;
    int getGlobalNumberOfTrees() const;
    int getNumberOfStateUpdatesWithoutAnySplit() const;
    int getMaximumTreesPerRank() const;

    void incLocalNumberOfSplits( int delta=1 );

    void notifyOfStateChange( State state );

    /**
     * If the stats spot some inconsistencies (local number of cells is bigger
     * than global number, e.g.) this means that the non-blocking data exchange
     * is lagging behind. In this case, this routine returns false;
     */
    bool hasConsistentViewOfWorld() const;

  private:
    static tarch::logging::Log _log;

    /**
     * Required for local stats, but also replicated in cell count metrics
     */
    int _localNumberOfInnerUnrefinedCells;

    /**
     * Required for local stats, but also replicated in cell count metrics
     */
    int _globalNumberOfInnerUnrefinedCells;

    int _globalNumberOfTrees;

    int _globalNumberOfRanksWithEnabledLoadBalancing;

    bool _localLoadBalancingEnabled;

    /**
     * This is my local accumulator where I keep track of how often I did
     * split in this iteration. At the end of each iteration, I roll this
     * one over into global or send it out.
     */
    int _localNumberOfSplits;

    /**
     * Lags behind global number by one iteration in an MPI world as data
     * will arrive with one iteration delay.
     */
    int _globalNumberOfSplits;

    int _numberOfStateUpdatesWithoutAnySplit;

    int _maximumTreesPerRank;

    bool _viewConsistent;

    #ifdef Parallel
    /**
     * Required for local stats, but also replicated in cell count metrics
     */
    MPI_Request*    _globalSumRequest;
    MPI_Request*    _globalNumberOfSplitsRequest;
    MPI_Request*    _globalNumberOfTreesRequest;
    MPI_Request*    _globalNumberOfRanksWithEnabledLoadBalancingRequest;
    MPI_Request*    _globalMaximumTreesPerRankRequest;
    #endif

    int             _globalNumberOfInnerUnrefinedCellsBufferIn;
    int             _globalNumberOfInnerUnrefinedCellsBufferOut;
    int             _numberOfSplitsIn;
    int             _numberOfSplitsOut;
    int             _numberOfTreesIn;
    int             _numberOfTreesOut;
    int             _numberOfRanksWithEnabledLoadBalancingIn;
    int             _numberOfRanksWithEnabledLoadBalancingOut;
    int             _numberOfMaximumTreesOut;
    int             _maximumTreesIn;

};


