// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "toolbox/loadbalancing/AbstractLoadBalancing.h"
#include "toolbox/loadbalancing/loadbalancing.h"


namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      class NoLoadBalancing;
    } // namespace strategies
  }   // namespace loadbalancing
} // namespace toolbox


/**
 * No load balancing
 *
 * A dummy that you can use if you wanna have a class around with a proper
 * load balancing signature.
 */
class toolbox::loadbalancing::strategies::NoLoadBalancing: public toolbox::loadbalancing::AbstractLoadBalancing {
public:
  /**
   * Delegate to other constructor with nullptr arguments, as configuration
   * does not matter anyway.
   */
  NoLoadBalancing();

  /**
   * Only there to be compatible with other classes.
   *
   * The important detail here is that the load balancing is switched off
   * right from the start.
   */
  NoLoadBalancing(Configuration* configuration, CostMetrics* costMetrics);


  virtual ~NoLoadBalancing() = default;

  /**
   * Finish a mesh sweep
   *
   * This
   */
  virtual void finishStep() override;

  /**
   * You cannot enable the no load balancing. This routine therefore sets
   * value to false and calls the superclass.
   */
  virtual void enable(bool value) override;
};
