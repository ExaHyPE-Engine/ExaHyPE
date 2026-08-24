// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "RuntimeParameters.h"

#include "runner/RuntimeParameters.h"

#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementCriterionHelpers.h"

#include "peano4/Globals.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/BooleanSemaphore.h"

{% if USE_VARIABLE_SHORTCUTS is sameas True %}
#include "VariableShortcuts{{SOLVER_NAME}}.h"
{% endif %}

{{SOLVER_INCLUDES}}

namespace {{NAMESPACE | join("::")}} {
  class {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} : public ::exahype2::Solver {
  public:
{% if USE_VARIABLE_SHORTCUTS is sameas True %}
    /**
     * Enables shortcuts to access variables, e.g. Q[Shortcuts::{{VARIABLE_NAMES[0]}}] instead of Q[0].
     */
    using Shortcuts = {{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}};
{% endif %}

    enum class SolverState {
      GridConstruction,
      GridInitialisation,

      TimeStep,

      Plotting,

      Suspended
    };

    static std::string toString(const SolverState state);

    static constexpr int NumberOfFiniteVolumesPerAxisPerPatch = {{NUMBER_OF_VOLUMES_PER_AXIS}};
    static constexpr int NumberOfUnknowns                     = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables           = {{NUMBER_OF_AUXILIARY_VARIABLES}};

    {{SOLVER_CONSTANTS}}

    {{CLASSNAME}}();
    ~{{CLASSNAME}}() override;
    {{CLASSNAME}}(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}({{CLASSNAME}}&& other) = delete;
    {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) = delete;

    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp() const override final;
    double getMaxTimeStamp() const override final;
    double getMinTimeStepSize() const override final;
    double getMaxTimeStepSize() const override final;

    void startGridConstructionStep() override;
    void finishGridConstructionStep() override;

    void startGridInitialisationStep() override;
    void finishGridInitialisationStep() override;

    void startTimeStep() override;
    void finishTimeStep() override;

    void startPlottingStep() override;
    void finishPlottingStep() override;

    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;

    int getNumberOfDoFs() const override final;

    void update(
      const double timeStepSize,
      const double timeStamp,
      const double minPatchSize,
      const double maxPatchSize
    );
    void updateTimeStampAndPatchSize(
      const double timeStamp,
      const double minPatchSize,
      const double maxPatchSize
    );

    SolverState getSolverState() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    void startSimulation() override;
    void finishSimulation() override;

    void suspendSolversForOneGridSweep() override;

    {{ABSTRACT_SOLVER_USER_DECLARATIONS}}

  protected:
    SolverState _solverState;

    double _minTimeStamp;
    double _maxTimeStamp;

    double _minTimeStampThisTimeStep;
    double _maxTimeStampThisTimeStep;

    double _localMinTimeStampThisTimeStep;
    double _localMaxTimeStampThisTimeStep;

    double _minVolumeH;
    double _maxVolumeH;

    double _minVolumeHThisTimeStep;
    double _maxVolumeHThisTimeStep;

    double _minTimeStepSize;
    double _maxTimeStepSize;

    double _minTimeStepSizeThisTimeStep;
    double _maxTimeStepSizeThisTimeStep;

    tarch::multicore::BooleanSemaphore _semaphore;

  private:
    void updateTimeStampAndPatchSizeWithoutLock(
      const double timeStamp,
      const double minPatchSize,
      const double maxPatchSize
    );
};
