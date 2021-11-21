// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_STRATEGY_H_
#define _TARCH_MULTICORE_ORCHESTRATION_STRATEGY_H_


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Strategy;
    }
  }
}


/**
 * Interface
 */
class tarch::multicore::orchestration::Strategy {
  public:
    virtual ~Strategy() = default;

    virtual void startBSPSection() = 0;
    virtual void endBSPSection()   = 0;

    /**
     * Is not const, as I want strategies give the opportunity to adopt
     * decisions after each call.
     */
    virtual int getNumberOfTasksToHoldBack() = 0;
    virtual int getNumberOfTasksToFuse() = 0;
};

#endif
