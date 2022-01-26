// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_ALLONGPU_H_
#define _TARCH_MULTICORE_ORCHESTRATION_ALLONGPU_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class AllOnGPU;
    }
  }
}


/**
 * A hard coded strategy that can realise a few standard tasking patterns
 *
 *
 *
 */
class tarch::multicore::orchestration::AllOnGPU: public tarch::multicore::orchestration::Strategy {
  private:
    const int  _device;
    bool       _isInBspSection;
  public:
    AllOnGPU(int device);

    void startBSPSection() override;
    void endBSPSection()   override;

    /**
     * Within a BSP section, I hold all tasks back. Afterwards, I release all
     * of them.
     */
    int getNumberOfTasksToHoldBack() override;

    FuseInstruction getNumberOfTasksToFuseAndTargetDevice() override;

    /**
     * No
     */
    bool fuseTasksImmediatelyWhenSpawned() override;

    /**
     * No
     */
    bool processPendingTasksWhileWaitingInBSPSection() override;

};

#endif
