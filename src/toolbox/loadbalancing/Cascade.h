// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "LoadBalancing.h"
#include "State.h"


namespace toolbox {
  namespace loadbalancing {
    class Cascade;
  }
}


/**
 * Cascade of load balancing schemes
 *
 * We run through a series of load balancing schemes. If one is stagnating or
 * switching off, we switch to the next one.
 *
 * I tried to play around with variadic templates, but that did not work for
 * multiple reasons, so I went down the manual way.
 */
template<typename HostedLoadBalancing0, typename HostedLoadBalancing1>
class toolbox::loadbalancing::Cascade: public toolbox::loadbalancing::LoadBalancing {
  public:
    Cascade(
      Configuration* configuration,
      bool deleteInDestructor
    ):
      _configuration(configuration),
      _deleteInDestructor(deleteInDestructor),
      _hostedLoadBalancing0(hostedLoadBalancing0, false),
      _hostedLoadBalancing1(hostedLoadBalancing1, false) {
    }

    virtual ~Cascade() {
      if (deleteInDestructor) {
        delete _configuration;
      }
    }
  private:
    Configuration*        _configuration;
    bool                  _deleteInDestructor;

    HostedLoadBalancing0  _hostedLoadBalancing0;
    HostedLoadBalancing1  _hostedLoadBalancing1;
};
