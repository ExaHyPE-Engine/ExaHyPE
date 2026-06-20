// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "RuntimeParameters.h"

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

      // Switch solver state for coupling
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

    static constexpr int NumberOfFiniteVolumesPerAxisPerPatch = {{NUMBER_OF_VOLUMES_PER_AXIS}};
    static constexpr int NumberOfUnknowns                     = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables           = {{NUMBER_OF_AUXILIARY_VARIABLES}};

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

    virtual void startGridConstructionStep() override;
    virtual void finishGridConstructionStep() override;

    virtual void startGridInitialisationStep() override;
    virtual void finishGridInitialisationStep() override;

    virtual void startTimeStep() override;
    virtual void finishTimeStep() override;

    virtual void startPlottingStep() override;
    virtual void finishPlottingStep() override;

    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;

    virtual int getNumberOfDoFs() const override final;

    /**
     * Update the global solver state, i.e. inform the solver about some
     * updated global quantities.
     *
     * @see setTimeStepSize(double)
     */
    void update(double timeStepSize, double timeStamp, double minVolumeSize, double maxVolumeSize);

    SolverState getSolverState() const;

    virtual bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    virtual void startSimulation() override;
    virtual void finishSimulation() override;

    virtual void suspendSolversForOneGridSweep() override;

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
};

{# Empty line here #}
