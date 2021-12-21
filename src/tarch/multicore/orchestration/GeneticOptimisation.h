// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_GENETIC_OPTIMISATION_H_
#define _TARCH_MULTICORE_ORCHESTRATION_GENETIC_OPTIMISATION_H_


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
      std::pair<int,int> tasksToFuseAndTargetDevice;
      bool fuseTasksImmediatelyWhenSpawned;
      bool processPendingTasksWhileWaiting;

      Key(
        int  numberOfTasksToHoldBack_,
        std::pair<int,int> tasksToFuseAndTargetDevice_,
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

    void createInitialConfigurations();
    void normalise();
    void increaseProbabilityOfActiveConfiguration();
    void removeUnreasonableProbabilities();
    void pickNewActiveConfigurationRandomly();
  public:
    GeneticOptimisation();
    virtual ~GeneticOptimisation() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    std::pair<int,int> getNumberOfTasksToFuseAndTargetDevice()     override;
    bool fuseTasksImmediatelyWhenSpawned() override;
    bool processPendingTasksWhileWaitingInBSPSection() override;
};


#endif
