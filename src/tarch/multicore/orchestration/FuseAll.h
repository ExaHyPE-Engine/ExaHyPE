// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_FUSE_ALL_H_
#define _TARCH_MULTICORE_ORCHESTRATION_FUSE_ALL_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class FuseAll;
    }
  }
}


/**
 * Try to fuse all tasks throughout a BSP section
 *
 * One of the simplest examples of task fusion, where
 */
class tarch::multicore::orchestration::FuseAll: public tarch::multicore::orchestration::Strategy {
  private:
    const int  _numberOfTaskToFuse;
    bool       _isInBSPSection;
  public:
    FuseAll(int numberOfTaskToFuse=4);
    virtual ~FuseAll() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    int getNumberOfTasksToFuse()     override;
};

#endif
