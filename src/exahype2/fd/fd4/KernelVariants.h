// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



namespace exahype2 {
  namespace fd {
    namespace fd4 {
      enum class DifferentialSourceTermVariant {
        CentralDifferences,
        CentralDifferencesWithLopsidedAdvection,
        CentralDifferencesWithLimiter,
        CentralDifferencesWithLimiterAndLopsidedAdvection
      };
    }
  }
}

