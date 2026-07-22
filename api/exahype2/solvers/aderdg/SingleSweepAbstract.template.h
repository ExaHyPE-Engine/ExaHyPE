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

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public ::exahype2::Solver {
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

    static std::string toString(SolverState);

    static constexpr int Order                      = {{ORDER}};
    static constexpr int NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    static constexpr double CFL                     = {{CFL_SAFETY_FACTOR}};
    static constexpr double PNPM                    = {{CFL_ADER}};

    // User-defined solver constants
    {{SOLVER_CONSTANTS}}

{% if USE_POINT_SOURCE %}
    static constexpr int NumberOfPointSources = {{NUMBER_OF_POINT_SOURCES}};
    static double pointSourceLocation[NumberOfPointSources][DIMENSIONS];
    virtual void initPointSourceLocations([[maybe_unused]] double location[NumberOfPointSources][DIMENSIONS]) {}
{% endif %}

    {{CLASSNAME}}();
    ~{{CLASSNAME}}();

    /**
     * Flags the coordinate axes along which the domain is periodic.
     */
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
      const {{SOLUTION_STORAGE_PRECISION}} * NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&         x,
      const tarch::la::Vector<DIMENSIONS,double>&         h,
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

    /**
     * @return Observed sizes, not the admissible quantities
     */
    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;

    int getNumberOfDoFs() const override final;

    /**
     * Update the global solver state, i.e. feed updated global quantities into
     * the solver.
     *
     * @param timeStepSize Time step size to fold into the global extrema
     * @param timeStamp    Time stamp to fold into the global extrema
     * @param minCellSize  Smallest observed cell size
     * @param maxCellSize  Largest observed cell size
     */
    void update(double timeStepSize, double timeStamp, double minCellSize, double maxCellSize);

    SolverState getSolverState() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    void startSimulation() override;
    void finishSimulation() override;

    {{ABSTRACT_SOLVER_USER_DECLARATIONS}}

  protected:
    SolverState _solverState;

    double     _minTimeStamp;
    double     _maxTimeStamp;

    double     _minTimeStampThisTimeStep;
    double     _maxTimeStampThisTimeStep;

    double     _localMinTimeStampThisTimeStep;
    double     _localMaxTimeStampThisTimeStep;

    double     _minCellH;
    double     _maxCellH;

    double     _minCellHThisTimeStep;
    double     _maxCellHThisTimeStep;

    double     _minTimeStepSize;
    double     _maxTimeStepSize;

    double     _minTimeStepSizeThisTimeStep;
    double     _maxTimeStepSizeThisTimeStep;

    tarch::multicore::BooleanSemaphore _semaphore;
};
