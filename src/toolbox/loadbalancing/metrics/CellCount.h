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


class toolbox::loadbalancing::metrics::CellCount: public toolbox::loadbalancing::CostMetrics {
  public:
    virtual ~CellCount() = default;

    virtual std::string toString() const override;

    virtual void waitForGlobalDataExchange() override;

    virtual void updateGlobalView() override;

};

