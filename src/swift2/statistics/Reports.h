// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace swift2 {
  namespace statistics {
    /**
     * Report the search radius
     *
     * Degenerates to nop if it is not called by the global master.
     */
    template <typename Particle>
    void reportSearchRadiusVTDt(const std::string& particleName);
  } // namespace statistics
} // namespace swift2

#include "Reports.cpph"
