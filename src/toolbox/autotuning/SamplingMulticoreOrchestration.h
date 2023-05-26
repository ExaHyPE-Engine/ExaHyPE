// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "Strategy.h"

#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

#include <map>


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class GeneticOptimisation;
    }
  }
}


/**
 * A hard coded strategy that can realise a few standard tasking patterns
 *
 *
 *
 */
class tarch::multicore::orchestration::GeneticOptimisation: public tarch::multicore::orchestration::Strategy {
  public:
    struct Key {
      int  numberOfTasksToHoldBack;
      FuseInstruction fusionInstruction;
      bool fuseTasksImmediatelyWhenSpawned;
      bool processPendingTasksWhileWaiting;

      Key(
        int  numberOfTasksToHoldBack_,
        FuseInstruction fusionInstruction_,
        bool fuseTasksImmediatelyWhenSpawned_,
        bool processPendingTasksWhileWaiting_
      );

      bool operator<(const Key& other) const;

      std::string toString() const;
    };

  private:
    static tarch::logging::Log _log;

    struct Value {
      double  probability;
      tarch::timing::Measurement  measurement;

      Value(double probability_);
    };

    std::map< Key, Value >     _configurationVariants;

    Key                        _activeKey;
    tarch::timing::Watch       _currentWatch;
    double                     _lastDuration;
    int                        _bspRunThroughsWithActiveKey;
    int                        _currentDevice;

    void createInitialConfigurations();
    void normalise();
    void increaseProbabilityOfActiveConfiguration();
    void removeUnreasonableProbabilities();
    void pickNewActiveConfigurationRandomly();
  public:
    GeneticOptimisation();
    virtual ~GeneticOptimisation() = default;

    virtual void startBSPSection() override;
    virtual void endBSPSection()   override;
    virtual int getNumberOfTasksToHoldBack() override;
    virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice()     override;
    virtual bool fuseTasksImmediatelyWhenSpawned() override;
    virtual ExecutionPolicy getExecutionPolicyForForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) override;
};


