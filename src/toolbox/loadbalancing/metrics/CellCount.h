// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/loadbalancing/CostMetrics.h"

namespace toolbox {
  namespace loadbalancing {
    namespace metrics {
      class CellCount;
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
class toolbox::loadbalancing::metrics::CellCount: public toolbox::loadbalancing::CostMetrics {
  public:
    CellCount();
    virtual ~CellCount() = default;

    virtual std::string toString() const override;

    virtual void updateGlobalView() override;

    virtual double getCostOfLocalTree(int spacetreeNumber) const override;
};

