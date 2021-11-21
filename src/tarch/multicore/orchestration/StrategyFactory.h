// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_STRATEGY_FACTORY_H_
#define _TARCH_MULTICORE_ORCHESTRATION_STRATEGY_FACTORY_H_


#include <string>


namespace tarch {
  namespace multicore {
    namespace orchestration {
      /**
       * Forward declaration
       */
      class Strategy;

      /**
       * Use toString() to see valid options
       */
      Strategy* parseRealisation( const std::string& realisation );

      std::string getListOfRealisations();

      /**
       * My default strategy is native.
       */
      Strategy* createDefaultStrategy();
    }
  }
}


#endif

