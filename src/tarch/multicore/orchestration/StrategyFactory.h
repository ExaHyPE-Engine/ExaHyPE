// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

#include "AllOnGPU.h"
#include "BackfillAndDeployRoundRobin.h"
#include "Hardcoded.h"

namespace tarch {
  namespace multicore {
    namespace orchestration {
      /**
       * Forward declaration
       */
      class Strategy;

      /**
       * Parse the realisation string
       *
       * Use toString() to see valid options. The result can be set active by
       * passing the object on to tarch::multicore::setOrchestration().
       */
      Strategy* parseRealisation(const std::string& realisation);

      std::string getListOfRealisations();

      /**
       * My default strategy is BackfillAndDeployRoundRobin() with the default
       * to fuse 16 tasks into one (GPU) task.
       */
      Strategy* createDefaultStrategy();
    } // namespace orchestration
  }   // namespace multicore
} // namespace tarch
