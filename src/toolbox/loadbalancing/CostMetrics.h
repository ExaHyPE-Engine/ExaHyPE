// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace toolbox {
  namespace loadbalancing {
    class CostMetrics;
  }
}


class toolbox::loadbalancing::CostMetrics {
  public:
    virtual ~CostMetrics() = default;

    virtual std::string toString() const = 0;

    /**
     * Typically called by
     *
     * - finishSimulation() and
     * - finishStep()
     *
     * Should wrap up any pending MPI stuff.
     */
    virtual void waitForGlobalDataExchange() = 0;

    virtual void updateGlobalView() = 0;
};
