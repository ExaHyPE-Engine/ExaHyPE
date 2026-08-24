// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "RuntimeParameters.h"

#include "runner/RuntimeParameters.h"

#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementCriterionHelpers.h"
#include "exahype2/PrecisionControl.h"
{%- if SUBCELL_LIMITER %}
#include "exahype2/dg/LimitingSolverState.h"
{%- endif %}

#include "peano4/Globals.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/BooleanSemaphore.h"

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
     * Enables shortcuts to access variables, e.g. Q[Shortcuts::{{VARIABLE_NAMES[0]}}] instead of Q[0].
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

    static std::string toString(const SolverState state);

    static constexpr int Order                      = {{ORDER}};
    static constexpr int NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    static constexpr double CFL                     = {{CFL_SAFETY_FACTOR}};
    static constexpr double PNPM                    = {{CFL_ADER}};

    {{SOLVER_CONSTANTS}}

{% if USE_POINT_SOURCE %}
    static constexpr int NumberOfPointSources = {{NUMBER_OF_POINT_SOURCES}};
    static double pointSourceLocation[NumberOfPointSources][DIMENSIONS];
    virtual void initPointSourceLocations([[maybe_unused]] double location[NumberOfPointSources][DIMENSIONS]) {}
{% endif %}

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

    {% if ADAPTIVE_PRECISION_IMPLEMENTATION!="<none>" %}
    /**
     * Evaluate the precision criterion over a cell.
     *
     * @param Q             Vector of unknowns
     * @param x             Centre of underlying cell
     * @param h             Size of underlying cell
     * @param t             Time
     * @param currentState  State of the solver at time of calling
     */
    virtual ::exahype2::PrecisionCommand precisionCriterion(
      const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&        x,
      const tarch::la::Vector<DIMENSIONS, double>&        h,
      const double                                        t,
      const SolverState                                   currentState
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
      const double timeStepSize,
      const double timeStamp,
      const double minCellSize,
      const double maxCellSize,
      const bool   hasHangingFaces
    );

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
    SolverState _solverState;
{%- if SUBCELL_LIMITER %}

    ::exahype2::dg::LimitingSweep _limitingSweep;

    std::atomic<long> _numberOfTroubledCellsThisTimeStep{0};
{%- endif %}

    double _minTimeStamp;
    double _maxTimeStamp;

    double _minTimeStampThisTimeStep;
    double _maxTimeStampThisTimeStep;

    double _localMinTimeStampThisTimeStep;
    double _localMaxTimeStampThisTimeStep;

    double _minCellH;
    double _maxCellH;

    double _minCellHThisTimeStep;
    double _maxCellHThisTimeStep;
    bool   _hasHangingFacesThisTimeStep;

    double _minTimeStepSize;
    double _maxTimeStepSize;

    double _minTimeStepSizeThisTimeStep;
    double _maxTimeStepSizeThisTimeStep;

    tarch::multicore::BooleanSemaphore _semaphore;
};
