// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "exahype2/RuntimeParameters.h"
#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"

#include "peano4/Globals.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <algorithm>

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
    /*
     * Enables the usage of shortcuts to access variables, e.g. use Q[Shortcuts::{{VARIABLE_NAMES[0]}}] instead of Q[0]
     */
    using Shortcuts = {{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}};
    {% endif %}

    enum class SolverState {
      GridConstruction,
      GridInitialisation,

      // switch solver state for coupling
      StillNeedsToSolveAfterGridInitialisation,
      StillNeedsToSolve,
      IsSolvingAfterGridInitialisation,
      IsSolving,
      HasSolved,

      Plotting,
      PlottingAfterGridInitialisation,

      Suspended
    };

    static std::string toString(SolverState);

    static constexpr int NumberOfNodesPerAxisPerPatch = {{NUMBER_OF_NODES_PER_AXIS}};
    static constexpr int NumberOfUnknowns             = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables   = {{NUMBER_OF_AUXILIARY_VARIABLES}};

    {{SOLVER_CONSTANTS}}

    {{CLASSNAME}}();
    ~{{CLASSNAME}}() = default;

    /**
     * Alias for periodic boundary conditions.
     */
    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp() const final;
    double getMaxTimeStamp() const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void startGridConstructionStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void finishGridConstructionStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void startGridInitialisationStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void finishGridInitialisationStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void startTimeStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void finishTimeStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void startPlottingStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void finishPlottingStep() override;

    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;
    double getMaxH() const;
    double getMinH() const;

    virtual int getNumberOfDoFs() const override final;

    /**
     * Update the global solver state, i.e. inform the solver about some
     * updated global quantities.
     *
     * @see setTimeStepSize(double)
     */
    void update(double timeStepSize, double timeStamp, double minH, double maxH);

    SolverState getSolverState() const;

    /**
     * Always holds.
     */
    virtual bool mayPlot() const override;

    /**
     * This predicate is always true, as we work with a single-sweep
     * implementation, i.e. each grid sweep realises one time step.
     */
    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    /**
     * Feel free to overwrite in user code, but ensure the superclass
     * implementation is still invoked, too.
     */
    virtual void startSimulation() override;

    virtual void suspendSolversForOneGridSweep() override;

    /**
     * Feel free to overwrite in user code, but ensure the superclass
     * implementation is still invoked, too.
     */
    virtual void finishSimulation() override;

    {{ABSTRACT_SOLVER_USER_DECLARATIONS}}

  protected:

    SolverState _solverState;

    double _minTimeStamp;
    double _maxTimeStamp;

    double _minTimeStampThisTimeStep;
    double _maxTimeStampThisTimeStep;

    double _localMinTimeStampThisTimeStep;
    double _localMaxTimeStampThisTimeStep;

    double _minH;
    double _maxH;

    double _minHThisTimeStep;
    double _maxHThisTimeStep;

    double _minTimeStepSize;
    double _maxTimeStepSize;

    double _minTimeStepSizeThisTimeStep;
    double _maxTimeStepSizeThisTimeStep;

    tarch::multicore::BooleanSemaphore _semaphore;
};

{# Empty line here #}
