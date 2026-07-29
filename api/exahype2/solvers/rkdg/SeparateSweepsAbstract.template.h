// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

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
   * Enables the usage of shortcuts to access variables, e.g. use Q[Shortcuts::{{VARIABLE_NAMES[0]}}] instead of Q[0].
   */
  using Shortcuts = {{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}};
{% endif %}

    enum class SolverState {
      GridConstruction,
      GridInitialisation,
      Suspended,
      Plotting,
      DynamicMeshRefinement {% for item in range(RK_STEPS) -%}
      ,
      ProjectOnFacesAndComputeVolumeIntegralOfStep{{item}},
      SolveRiemannProblemAndAddToVolumeIntegralOfStep{{item}}
      {%- endfor %}
    };

    static std::string toString(SolverState);

    static constexpr int  DGOrder = {{DG_ORDER}};

    static constexpr int    NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int    NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};

{{BASIS_DECLARATIONS | indent(4,True) }}

    {{SOLVER_CONSTANTS}}

    {{CLASSNAME}}();

    /**
     * Flags which coordinate axes use periodic boundary conditions.
     */
    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp() const override final;
    double getMaxTimeStamp() const override final;
    double getMinTimeStepSize() const override final;
    double getMaxTimeStepSize() const override final;

    /**
     * Evaluate the refinement criterion over one cell patch.
     *
     * @param Q Whole nodal patch in AoS layout, or nullptr during grid construction.
     * @param x Patch centre.
     * @param h Patch size.
     * @param t Time.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double * NOALIAS                  Q,
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      const tarch::la::Vector<DIMENSIONS,double>&  h,
      double                                       t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    /**
     * Set the initial condition in one nodal point.
     *
     * @param x Position of point that is to be initialised within computational domain.
     * @param h Size of cell within which the point is placed.
     */
    virtual void initialCondition(
      double * NOALIAS                        Q,     // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      const tarch::la::Vector<DIMENSIONS,double>&  h
    ) {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<none>" %}
    {% else %}
    /**
     * Apply boundary conditions. You can overwrite both the inside and
     * outside values, though most boundary conditions modify only the
     * outside ones. The values set here are subsequently passed through
     * the Riemann solver, i.e. flux and eigenvalues.
     */
    virtual void boundaryConditions(
      const double * NOALIAS                  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double * NOALIAS                        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      double                                       t,
      int                                          normal
    ) {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}

    void suspendSolversForOneGridSweep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startGridConstructionStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishGridConstructionStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startGridInitialisationStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishGridInitialisationStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startTimeStep() override;

    /**
     * The dedicated DynamicAMR observer realises the committed refine/erase events between
     * time steps. Every Runge-Kutta exchange predicate keys on the solver state, so the AMR
     * sweeps run under a state of their own and no step-bound face or cell exchange fires.
     */
    void startDynamicMeshRefinementStep() override;
    void finishDynamicMeshRefinementStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishTimeStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startPlottingStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishPlottingStep() override;

    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;

    int getNumberOfDoFs() const override final;

    /**
     * Update the global solver state, i.e. inform the solver about some
     * updated global quantities.
     *
     * @see setTimeStepSize(double)
     */
    void update(double timeStepSize, double timeStamp, double cellSize);

    SolverState  getSolverState() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startSimulation() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void finishSimulation() override;

    {% if STATELESS_PDE_TERMS %}
    /**
     * By default each cell can use stateless PDE terms, i.e. the static
     * versions of the operators. Therefore, each cell can go to the GPU
     * or inline aggressively. You can disable this behaviour for selected
     * cells via this callback.
     */
    {% else %}
    /**
     * Warning: This operation has no semantics, as you have disabled
     * stateless PDE terms. The routine is here to allow you to write one
     * code version for both a GPU-enabled solver and a solver without
     * GPU support.
     */
    {% endif %}
    virtual bool cellCanUseStatelessPDETerms(
      const tarch::la::Vector<DIMENSIONS,double>&  cellCentre,
      const tarch::la::Vector<DIMENSIONS,double>&  cellH,
      double                                       t,
      double                                       dt
    ) const;

  protected:
    SolverState  _solverState;
    SolverState  _solverStateBeforeDynamicMeshRefinement;

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

    tarch::multicore::BooleanSemaphore  _semaphore;

  {{ABSTRACT_SOLVER_USER_DECLARATIONS}}
};
