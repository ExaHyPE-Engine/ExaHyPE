// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"


namespace toolbox {
  namespace loadbalancing {
    class NoLoadBalancing;
  }
}


/**
 * No load balancing
 *
 * A dummy that you can use if you wanna have a class around with a proper
 * load balancing signature.
 */
class toolbox::loadbalancing::NoLoadBalancing {
  public:
	NoLoadBalancing();

    void finishStep();

    void finishSimulation();

    /**
     * I need the stats here mainly for debugging purposes.
     */
    void dumpStatistics();

    bool hasSplitRecently() const;

    void enable(bool);
    
    /**
     * Local operation
     */
    bool isEnabled(bool globally) const;

    /**
     * @return -1 if this information is not available
     */
    int getGlobalNumberOfTrees() const;
};



