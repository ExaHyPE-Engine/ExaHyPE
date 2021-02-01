// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_LOAD_BALANCING_NO_LOAD_BALANCING_
#define _TOOLBOX_LOAD_BALANCING_NO_LOAD_BALANCING_



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
};


#endif

