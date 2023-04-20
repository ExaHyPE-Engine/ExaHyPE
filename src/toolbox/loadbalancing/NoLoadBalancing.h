// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"

#include "AbstractLoadBalancing.h"


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
class toolbox::loadbalancing::NoLoadBalancing: public toolbox::loadbalancing::LoadBalancing {
  public:
    NoLoadBalancing();

    /**
     * Only there to be compatible with other classes.
     */
    NoLoadBalancing(
      Configuration* configuration,
      bool deleteInDestructor
    );
    virtual ~NoLoadBalancing() = default;

    void finishStep() override;
};



