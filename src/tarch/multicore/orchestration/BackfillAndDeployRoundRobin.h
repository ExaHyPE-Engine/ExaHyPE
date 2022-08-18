// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_BackfillAndDeployRoundRobin_H_
#define _TARCH_MULTICORE_ORCHESTRATION_BackfillAndDeployRoundRobin_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class BackfillAndDeployRoundRobin;
    }
  }
}


/**
 * A hard coded strategy that can realise a few standard tasking patterns
 *
 *
 *
 */
class tarch::multicore::orchestration::BackfillAndDeployRoundRobin: public tarch::multicore::orchestration::Strategy {
  private:
    int  _minTasksToFuse;
    int  _nextDeviceToUse;
    bool _inBSPSection;

    void toggleDevice();
  public:
    BackfillAndDeployRoundRobin(
      int  minTasksToFuse
    );
    virtual ~BackfillAndDeployRoundRobin() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    FuseInstruction getNumberOfTasksToFuseAndTargetDevice()     override;
    bool fuseTasksImmediatelyWhenSpawned() override;
    bool processPendingTasksWhileWaitingInBSPSection() override;
};

#endif
