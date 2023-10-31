// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace toolbox {
  namespace loadbalancing {
    class CostMetrics;
  }
}



#include "tarch/mpi/mpi.h"
#include "tarch/logging/Log.h"
#include <string>


/**
 * Abstract cost metric
 *
 * Base class for any cost metric that you want to use in combination with the
 * generic load balancing strategies. To realise your own cost metric, you have
 * to complete two steps:
 *
 * 1. The most important quantity of this class is _localRankWeight which is
 *    not set by this abstract base class. I recommend to redefine the
 *    routine updateGlobalView(), to set this attribute there, and then to call
 *    CostMetric's updateGlobalView() to finish all global data exchange.
 * 2. Next, you have to implement getCostOfLocalTree().
 *
 */
class toolbox::loadbalancing::CostMetrics {
  public:
    CostMetrics();
    virtual ~CostMetrics() = default;

    /**
     * Feel free to invoke the variant below if you want. Or add additional info.
     */
    virtual std::string toString() const = 0;

    virtual std::string toString(const std::string& metricName) const;

    /**
     * Typically called by
     *
     * - finishSimulation() and
     * - finishStep()
     *
     * Should wrap up any pending MPI stuff. If you add your own collectives
     * in a subclass, overwrite it, but still invoke the superclass.
     */
    virtual void waitForGlobalDataExchange();

    /**
     * Please overwrite in subclass and set the value of _localRankWeight.
     * Afterwards, call this superclass routine
     */
    virtual void updateGlobalView();

    /**
     * Query cost of one tree
     *
     * This routine is only called by the rank which owns the tree
     * spacetreeNumber. That is, you don't need global knowledge of the
     * tree weight distribution.
     *
     * The routine is used in multiple places, but the most important one is
     * toolbox::loadbalancing::AbstractLoadBalancing::getIdOfHeaviestLocalSpacetree(),
     * which analyses the whole tree cost distribution on a rank.
     */
    virtual double getCostOfLocalTree(int spacetreeNumber) const = 0;

    /**
     * Wrapper around getCostOfLocalTree(). Loops over all local trees and
     * returns the sum.
     */
    virtual double getCostOfLocalRank() const;

    virtual double getGlobalCost() const;

    virtual int getLightestRank() const;

    virtual double getMinimumOfMaximumRankWeights() const;

  protected:
    /**
     * It is totally annoying, but it seems that MPI's maxloc and reduction are broken
     * in some MPI implementations.
     */
    struct ReductionBuffer {
      double   _weight;
      int      _rank;
    };

    /**
     * Weight of local rank
     *
     * Weight of the whole rank. This quantity is the key quantity
     * whenever we actually ask the metrics for information, as it
     * feeds into the total weight, but also helps us to identify
     * underbooked ranks.
     */
    double _localRankWeight;

    double           _globalWeight;
    ReductionBuffer  _lightestRank;
    double           _minimumOfMaximumOfRankWeights;

    #ifdef Parallel
    /**
     * Replicate compared to stats
     */
    MPI_Request*    _globalWeightRequest;
    MPI_Request*    _lightestRankRequest;
    MPI_Request*    _minimumOfMaximumOfRankWeightsRequest;
    #endif

    double          _globalWeightIn;
    double          _globalWeightOut;

    ReductionBuffer _lightestRankIn;
    ReductionBuffer _lightestRankOut;

    double          _minimumOfMaximumOfRankWeightsIn;
    double          _minimumOfMaximumOfRankWeightsOut;
  private:
    static tarch::logging::Log _log;
};
