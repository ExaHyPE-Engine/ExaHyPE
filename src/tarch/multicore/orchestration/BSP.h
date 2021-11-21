// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_BSP_H_
#define _TARCH_MULTICORE_ORCHESTRATION_BSP_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class BSP;
    }
  }
}


/**
 * Interface
 */
class tarch::multicore::orchestration::BSP: public tarch::multicore::orchestration::Strategy {
  public:
    virtual ~BSP() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    int getNumberOfTasksToFuse()     override;
};

#endif
