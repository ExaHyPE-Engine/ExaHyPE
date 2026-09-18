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

#include "peano4/Constants.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

{% if USE_VARIABLE_SHORTCUTS is sameas True %}
#include "VariableShortcuts{{SOLVER_NAME}}.h"
{% endif %}

#include "kernels/{{SOLVER_NAME}}/ButcherTableau.h"
#include "kernels/{{SOLVER_NAME}}/DGMatrices.h"
#include "kernels/{{SOLVER_NAME}}/Quadrature.h"

{{SOLVER_INCLUDES}}

namespace {{NAMESPACE | join("::")}} {
  class {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} : public ::exahype2::Solver {
  public:
{% if USE_VARIABLE_SHORTCUTS is sameas True %}
    /**
     * Names the variable indices, so Q[Shortcuts::{{VARIABLE_NAMES[0]}}] reads the first variable.
     */
    using Shortcuts = {{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}};
{% endif %}

    enum class SolverState {
      GridConstruction,
      GridInitialisation,
      Suspended,
      Plotting,
      PlottingAfterGridInitialisation,
      TimeStepAfterGridInitialisation,
      DynamicMeshRefinement {% for item in range(RK_STEPS) -%}
      , RungeKuttaStep{{item}}
      {%- endfor %}
    };

    static std::string toString(SolverState state);

    static constexpr int DGOrder = {{DG_ORDER}};
    static constexpr int RKOrder = {{RK_ORDER}};

    static constexpr int NumberOfUnknowns = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};

    {{SOLVER_CONSTANTS}}

    {{CLASSNAME}}();
    {{CLASSNAME}}(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}({{CLASSNAME}}&& other) = delete;
    {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) = delete;
    ~{{CLASSNAME}}() override;

    /**
     * Flags which coordinate axes use periodic boundary conditions.
     */
    static std::bitset<DIMENSIONS> PeriodicBoundaries;

    double getMinTimeStamp() const override final;
    double getMaxTimeStamp() const override final;
    double getMinTimeStepSize() const override final;
    double getMaxTimeStepSize() const override final;

    void suspendSolversForOneGridSweep() override;

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
      double timeStepSize,
      double timeStamp,
      double cellSize
    );

    void reduceLocalUpdates();

    SolverState  getSolverState() const;

    int getRkStage() const;

    int getRkStageAfterDynamicMeshRefinement() const;

    double getPreviousStageTimeStepSize() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    bool usesDynamicMeshRefinementSweeps() const;

    void startSimulation() override;
    void finishSimulation() override;

  protected:
    /** Stores one traversal's extrema until reduceLocalUpdates() reduces them. */
    struct LocalUpdates final {
      double                            minTimeStamp{std::numeric_limits<double>::infinity()};
      double                            maxTimeStamp{0.0};
      double                            localMinTimeStamp{std::numeric_limits<double>::infinity()};
      double                            localMaxTimeStamp{0.0};
      double                            minCellH{std::numeric_limits<double>::infinity()};
      double                            maxCellH{0.0};
      ::exahype2::NonNegativeStatistics timeStepSize{};
      bool                              empty{true};
    };

    static thread_local LocalUpdates _localUpdates;

    SolverState _solverState{SolverState::GridConstruction};
    SolverState _solverStateAfterDynamicMeshRefinement{SolverState::RungeKuttaStep0};
    bool        _usesDynamicMeshRefinementSweeps{false};
    double      _previousStageTimeStepSize{0.0};

    double _minTimeStamp{0.0};
    double _maxTimeStamp{0.0};

    double _minTimeStampThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxTimeStampThisTimeStep{0.0};

    double _localMinTimeStampThisTimeStep{std::numeric_limits<double>::infinity()};
    double _localMaxTimeStampThisTimeStep{0.0};

    double _minCellH{std::numeric_limits<double>::infinity()};
    double _maxCellH{0.0};

    double _minCellHThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxCellHThisTimeStep{0.0};

    double _minTimeStepSize{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSize{0.0};

    double _minTimeStepSizeThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSizeThisTimeStep{0.0};

  {{ABSTRACT_SOLVER_USER_DECLARATIONS}}
};
