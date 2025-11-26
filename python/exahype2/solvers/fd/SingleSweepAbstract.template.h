// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"

#include "peano4/utils/Globals.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <algorithm>

{% if USE_VARIABLE_SHORTCUT is sameas True %}
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
    {% if USE_VARIABLE_SHORTCUT is sameas True %}
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

    /**
     * Maximum mesh size that this solver wants/is allowed to support. Peano 4
     * has to ensure that none of its ExaHyPE 2 solvers has to work on a mesh
     * which is coarser than its MaxH value.
     */
    static constexpr double MaxAdmissiblePatchH  = {{MAX_H}} * NumberOfNodesPerAxisPerPatch;
    static constexpr double MaxAdmissibleH = {{MAX_H}};
    static constexpr double MaxAdmissibleCellH   = MaxAdmissiblePatchH;

    /**
     * Minimum mesh size that this solver wants to support. Peano 4 tries
     * to ensure that none of its ExaHyPE 2 solvers has to work on a mesh
     * which is finer than its MinH value. However, if there are multiple
     * solvers, the interplay of these solvers might imply that some solvers
     * work with fine mesh resolutions even though they have specified a
     * coarser MinH.
     */
    static constexpr double MinAdmissiblePatchH  = {{MIN_H}} * NumberOfNodesPerAxisPerPatch;
    static constexpr double MinAdmissibleH = {{MIN_H}};
    static constexpr double MinAdmissibleCellH   = MinAdmissiblePatchH;

    {{SOLVER_CONSTANTS}}

    {{CLASSNAME}}();
    ~{{CLASSNAME}}() = default;

    /**
     * Alias for periodic boundary conditions.
     */
    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp(bool ofCurrentlyRunningGridSweep=false) const final;
    double getMaxTimeStamp(bool ofCurrentlyRunningGridSweep=false) const final;
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

    virtual void printTimeStep(bool ofCurrentlyRunningGridSweep) override;

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

    /**
     * @param currentTimeStep If you set this to false, you'll get the
     *   quantity from the preceding time step. This is important for
     *   local time stepping with fixed subcycling, as they have to
     *   know the sizes from the last time step throughout the traversal,
     *   where the current patch size might just be re-evaluated.
     *
     * @return Actually observed sizes, not the admissible quantities
     */
    double getMaxPatchSize(bool currentTimeStep = true) const;
    double getMinPatchSize(bool currentTimeStep = true) const;
    double getMaxH(bool currentTimeStep = true) const;
    double getMinH(bool currentTimeStep = true) const;

    /**
     * Within the FV context, mesh is an alias for patch.
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
    static tarch::logging::Log _log;

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
