// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "RuntimeParameters.h"

#include "exahype2/RuntimeParameters.h"
#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/PrecisionControl.h"

#include "peano4/Globals.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include "kernels/{{SOLVER_NAME}}/Quadrature.h"
#include "kernels/{{SOLVER_NAME}}/DGMatrices.h"

{% if USE_HALF_PRECISION==True %}
#include <stdfloat>
{% endif %}

{% if USE_VARIABLE_SHORTCUTS is sameas True %}
#include "VariableShortcuts{{SOLVER_NAME}}.h"
{% endif %}

{{SOLVER_INCLUDES}}

#include <algorithm>

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
    /*
     * Enables the usage of shortcuts to access variables, e.g. use Q[Shortcuts::{{VARIABLE_NAMES[0]}}] instead of Q[0]
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
{% endif %}

    {{CLASSNAME}}();
    ~{{CLASSNAME}}();

    /**
     * Alias for periodic boundary conditions.
     */
    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp() const final;
    double getMaxTimeStamp() const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;

    {% if ADAPTIVE_PRECISION_IMPLEMENTATION!="<none>" %}
    /**
     * Evaluate precision criterion over cell
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

    virtual void startGridConstructionStep() override;
    virtual void finishGridConstructionStep() override;

    virtual void startGridInitialisationStep() override;
    virtual void finishGridInitialisationStep() override;

    virtual void suspendSolversForOneGridSweep() override;

    virtual void startTimeStep() override;
    virtual void finishTimeStep() override;

    virtual void logTimeStep() override;

    virtual void startPlottingStep() override;
    virtual void finishPlottingStep() override;

    /**
     * @return Actually observed sizes, not the admissible quantities
     */
    double getMaxCellSize() const;
    double getMinCellSize() const;

    /**
     * Within the DG context, mesh is an alias for cell.
     * TODO: Patch is probably the better name than mesh.
     */
    virtual double getMaxMeshSize() const override final;
    virtual double getMinMeshSize() const override final;

    virtual int getNumberOfDoFs() const override final;

    /**
     * Update the global solver state, i.e. inform the solver about some
     * updated global quantities.
     *
     * @see setTimeStepSize(double)
     */
    void update(double timeStepSize, double timeStamp, double minCellSize, double maxCellSize);

    SolverState getSolverState() const;

    virtual bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    virtual void startSimulation() override;
    virtual void finishSimulation() override;

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
