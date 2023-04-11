// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"
#include "tarch/mpi/mpi.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "Configuration.h"
#include "Blacklist.h"
#include "Statistics.h"


namespace toolbox {
  namespace loadbalancing {
    class LoadBalancing;
  }
}

class toolbox::loadbalancing::LoadBalancing {
  public:
    /**
     * Data ownership is handed over to the load balancing, i.e. the balancing
     * has to delete the configuration object.
     */
    LoadBalancing(Configuration* configuration = new DefaultConfiguration());
    virtual ~LoadBalancing();

    /**
     * Generic string serialisation. You might want to extend it. Therefore,
     * the routine is labelled as virtual.
     */
    virtual std::string toString() const;

    /**
     * Switch on/off.
     */
    void enable(bool);

    bool hasSplitRecently() const;

    /**
     * Clarifies whether load balancing is, in principle, enabled. Might however
     * mean that the load balancing is currently (temporarily) disabled. In
     * this case, the routine still returns true.
     */
    bool isEnabled(bool globally) const;

    int getGlobalNumberOfTrees() const;

    virtual void finishSimulation();
  protected:
    static tarch::logging::Log  _log;

    Blacklist   _blacklist;

    Statistics  _statistics;

    Configuration*  _configuration;

    State  _state;

    /**
     * Ensure enough memory is left-over.
     */
    bool fitsIntoMemory(State state) const;
};

