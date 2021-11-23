// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_FUSE_H_
#define _TARCH_MULTICORE_ORCHESTRATION_FUSE_H_


#include "Strategy.h"
#include "tarch/multicore/Tasks.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Fuse;
    }
  }
}


/**
 * Try to fuse all tasks throughout a BSP section
 *
 * One of the simplest examples of task fusion, where
 */
class tarch::multicore::orchestration::Fuse: public tarch::multicore::orchestration::Strategy {
  private:
    const int  _numberOfTaskToFuse;
    const int  _targetDevice;
    bool       _isInBSPSection;
  public:
    Fuse(int numberOfTaskToFuse, int targetDevice=tarch::multicore::Task::Host);
    virtual ~Fuse() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    std::pair<int,int> getNumberOfTasksToFuseAndTargetDevice() override;
};

#endif
