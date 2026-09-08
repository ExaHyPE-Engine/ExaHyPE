// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "RuntimeParameters.h"

#include "runner/RuntimeParameters.h"

#include "exahype2/NonNegativeStatistics.h"
#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementCriterionHelpers.h"

#include "peano4/Globals.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

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
     * Names the variables: Q[Shortcuts::{{VARIABLE_NAMES[0]}}] is Q[0].
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
    {{CLASSNAME}}(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}({{CLASSNAME}}&& other) = delete;
    {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) = delete;
    ~{{CLASSNAME}}() override;

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

    /**
     * Reduces the extrema the current traversal has accumulated through update()
     * into the solver. Runs once per tree at traversal end, so the per-patch
     * update writes thread-local data only.
     */
    void reduceLocalUpdates();

    SolverState getSolverState() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    void startSimulation() override;
    void finishSimulation() override;

    void suspendSolversForOneGridSweep() override;

    {{ABSTRACT_SOLVER_USER_DECLARATIONS}}

  protected:
    /** Stores one traversal's extrema until reduceLocalUpdates() reduces them. */
    struct LocalUpdates {
      double                            minTimeStamp{std::numeric_limits<double>::infinity()};
      double                            maxTimeStamp{0.0};
      double                            localMinTimeStamp{std::numeric_limits<double>::infinity()};
      double                            localMaxTimeStamp{0.0};
      double                            minVolumeH{std::numeric_limits<double>::infinity()};
      double                            maxVolumeH{0.0};
      ::exahype2::NonNegativeStatistics timeStepSize{};
      bool                              empty{true};
    };

    static thread_local LocalUpdates _localUpdates;

    SolverState _solverState{SolverState::GridConstruction};

    double _minTimeStamp{0.0};
    double _maxTimeStamp{0.0};

    double _minTimeStampThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxTimeStampThisTimeStep{0.0};

    double _localMinTimeStampThisTimeStep{std::numeric_limits<double>::infinity()};
    double _localMaxTimeStampThisTimeStep{0.0};

    double _minVolumeH{std::numeric_limits<double>::infinity()};
    double _maxVolumeH{0.0};

    double _minVolumeHThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxVolumeHThisTimeStep{0.0};

    double _minTimeStepSize{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSize{0.0};

    double _minTimeStepSizeThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSizeThisTimeStep{0.0};
};
