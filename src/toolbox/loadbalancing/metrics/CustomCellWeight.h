// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/loadbalancing/CostMetrics.h"

#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <map>


namespace toolbox {
  namespace loadbalancing {
    namespace metrics {
      class CustomCellWeight;
    }
  }
}


/**
 * Cost metrics based solely on cell counts
 *
 * This is a very simple implementations which basically is based upon the mesh
 * statistics that we have anyway. So the metrics do not require further user
 * input.
 */
class toolbox::loadbalancing::metrics::CustomCellWeight: public toolbox::loadbalancing::CostMetrics {
  public:
    CustomCellWeight();
    virtual ~CustomCellWeight() = default;

    virtual std::string toString() const override;

    /**
     * Update global view
     *
     * This routine runs through four steps:
     *
     * 1. We copy _currentCellWeights into _previousCellWeights erasing the
     *    latter.
     * 2. We clear _previousCellWeights, so it can be used to reaccumulate
     *    any data of interest.
     * 3. We determine the value of _localRankWeight.
     * 4. We finally invoke the superclass' routine so it can handle all the
     *    global data exchange.
     */
    virtual void updateGlobalView() override;

    /**
     * Implementation of abstract superclass routine
     *
     * Deliver the requested entry from _previousCellWeights. If no such entry
     * exists, we return 0.0. As this is a read-only access, we don't need a
     * semaphore.
     */
    virtual double getCostOfLocalTree(int spacetreeNumber) const override;

    /**
     * Report weight of one cell
     *
     * This routine is thread-safe, as it will be invoked by multiple tree
     * traversals at the same time. It accumulates data within
     * _currentCellWeights.
     */
    static void logCellWeight( int spacetreeNumber, double weight );
  private:
    static tarch::multicore::BooleanSemaphore  _semaphore;
    static tarch::logging::Log                 _log;

    /**
     * Mapping of trees onto cell weights. These are the one from a previous
     * tree traversal and hence complete data which we can use to read.
     */
    static std::map<int, double>  _previousCellWeights;

    /**
     * Mapping of trees onto cell weights. This is a working data structure,
     * i.e. one we use to accumulate data into. The current data hence might
     * still be incomplete when you read it. As we accumulate data from the
     * local rank only, we do not hold global information, i.e. information
     * from other ranks.
     */
    static std::map<int, double>  _currentCellWeights;
};

