// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LimiterStatus.h"

namespace {
  // Taking these addresses makes the core library compile every status operation.
  [[maybe_unused]] void instantiateLimiterStatusFunctions() {
    [[maybe_unused]] auto* runsSubcellScheme = &exahype2::dg::runsSubcellScheme;
    [[maybe_unused]] auto* mergeStatus       = &exahype2::dg::mergeLimiterStatus;
    [[maybe_unused]] auto* spreadStatus      = &exahype2::dg::spreadLimiterStatus;
    [[maybe_unused]] auto* loadStatus        = &exahype2::dg::loadFaceStatus;
    [[maybe_unused]] auto* storeStatus       = &exahype2::dg::storeFaceStatus;
    [[maybe_unused]] auto* raiseStatus       = &exahype2::dg::raiseFaceStatus;
  }
} // namespace
