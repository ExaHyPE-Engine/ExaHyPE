// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace toolbox::blockstructured {
  template <typename Value, typename Indicator>
  INLINE_METHOD double computeMultiresolutionDetail(
    const Value* const NOALIAS Q,
    const int                  numberOfDofsPerAxis,
    const int                  numberOfUnknownsPlusAuxiliaryVariables,
    const Indicator&           indicator,
    const double* const        nodesInUnitInterval = nullptr,
    const double               floor               = 1.0e-12
  );
} // namespace toolbox::blockstructured

#include "toolbox/blockstructured/MultiresolutionDetail.cpph"
