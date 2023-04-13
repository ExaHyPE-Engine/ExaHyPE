// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "LoadBalancing.h"


namespace toolbox {
  namespace loadbalancing {
    class SpreadOut;
  }
}



class toolbox::loadbalancing::SpreadOut: public toolbox::loadbalancing::LoadBalancing {
  public:
    SpreadOut(Configuration* configuration = new DefaultConfiguration());
    virtual ~SpreadOut();

    void finishStep() override;
  private:
    static tarch::logging::Log  _log;

    int _previousNumberOfCells;
    int _numberOfStableGridIterations;

    /**
     * Return 0 if the load balancing should not split (yet) and otherwise
     * return number of splits required.
     */
    int getNumberOfTreesPerRank() const;

    void updateLoadBalancing();

    void triggerSplit( int numberOfCells, int targetRank );
};

