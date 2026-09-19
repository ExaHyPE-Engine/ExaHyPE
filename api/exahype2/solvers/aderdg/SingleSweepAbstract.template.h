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
#include "exahype2/PrecisionControl.h"
{%- if SUBCELL_LIMITER %}
#include "exahype2/dg/LimitingSolverState.h"
{%- endif %}

#include "peano4/Constants.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

#include "kernels/{{SOLVER_NAME}}/Quadrature.h"
#include "kernels/{{SOLVER_NAME}}/DGMatrices.h"

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
     * Names the variable indices, so Q[Shortcuts::{{VARIABLE_NAMES[0]}}] reads the first variable.
     */
    using Shortcuts = {{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}};
{% endif %}

    enum class SolverState {
      GridConstruction,
      GridInitialisation,

      InitialPrediction,
      PredictionOnHangingCells,

      TimeStep,

      Plotting,

      Suspended
    };

    static std::string toString(SolverState state);

    static constexpr int DGOrder                    = {{ORDER}};
    static constexpr int NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    static constexpr double CFL                     = {{CFL_SAFETY_FACTOR}};
    static constexpr double PNPM                    = {{CFL_ADER}};
    /**
     * The Picard iteration of the predictor stops once the Euclidean distance between two
     * iterates falls below this value.
     */
    static constexpr double PicardResidualTolerance = {{PICARD_RESIDUAL_TOLERANCE}};

    {{SOLVER_CONSTANTS}}

{% if USE_POINT_SOURCE %}
    static constexpr int NumberOfPointSources = {{NUMBER_OF_POINT_SOURCES}};
    static double pointSourceLocation[NumberOfPointSources][DIMENSIONS];
    virtual void initPointSourceLocations([[maybe_unused]] double location[NumberOfPointSources][DIMENSIONS]) {}
{% endif %}

    {{CLASSNAME}}();
    {{CLASSNAME}}(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}({{CLASSNAME}}&& other) = delete;
    {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other) = delete;
    {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) = delete;
    ~{{CLASSNAME}}() override;

    static std::bitset<DIMENSIONS> PeriodicBoundaries;

    double getMinTimeStamp() const override final;
    double getMaxTimeStamp() const override final;
    double getMinTimeStepSize() const override final;
    double getMaxTimeStepSize() const override final;

    {% if ADAPTIVE_PRECISION_IMPLEMENTATION!="<none>" %}
    virtual ::exahype2::PrecisionCommand precisionCriterion(
      const {{SOLUTION_STORAGE_PRECISION}}* NOALIAS       Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&        x,
      const tarch::la::Vector<DIMENSIONS, double>&        h,
      double                                              t,
      SolverState                                         currentState
    ) {% if ADAPTIVE_PRECISION_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    void startGridConstructionStep() override;
    void finishGridConstructionStep() override;

    void startGridInitialisationStep() override;
    void finishGridInitialisationStep() override;

    void suspendSolversForOneGridSweep() override;

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
      double minCellSize,
      double maxCellSize,
      bool   hasHangingFaces
    );

    void reduceLocalUpdates();

    SolverState getSolverState() const;
{%- if SUBCELL_LIMITER %}

    ::exahype2::dg::LimitingSolverState getLimitingSolverState() const;

    void markCellAsTroubled();

    long getNumberOfTroubledCells() const;

    bool isFinalSubcellSpreadSweep() const;
{%- endif %}

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    void startSimulation() override;
    void finishSimulation() override;

    {{ABSTRACT_SOLVER_USER_DECLARATIONS}}

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
      bool                              hasHangingFaces{false};
      bool                              empty{true};
    };

    static thread_local LocalUpdates _localUpdates;

    SolverState _solverState{SolverState::GridConstruction};
{%- if SUBCELL_LIMITER %}

    ::exahype2::dg::LimitingSweep _limitingSweep;

    std::atomic<long> _numberOfTroubledCellsThisTimeStep{0};
{%- endif %}

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
    /** Every traversal task's reduceLocalUpdates() may raise the flag concurrently. */
    std::atomic<bool> _hasHangingFacesThisTimeStep{false};

    double _minTimeStepSize{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSize{0.0};

    double _minTimeStepSizeThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSizeThisTimeStep{0.0};
};
