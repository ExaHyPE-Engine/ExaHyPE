// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <bitset>

namespace peano4 {
  namespace utils {
    /**
     * Is used by the z-loop. See macro dforz.
     */
    typedef std::bitset<DIMENSIONS> LoopDirection;
  } // namespace utils
} // namespace peano4
