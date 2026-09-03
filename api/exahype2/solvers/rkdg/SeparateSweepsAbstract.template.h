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
      Suspended,
      Plotting,
      DynamicMeshRefinement {% for item in range(RK_STEPS) -%}
      ,
      ProjectOnFacesAndComputeVolumeIntegralOfStep{{item}},
      SolveRiemannProblemAndAddToVolumeIntegralOfStep{{item}}
      {%- endfor %}
    };

    static std::string toString(const SolverState state);

    static constexpr int DGOrder = {{DG_ORDER}};

    static constexpr int NumberOfUnknowns = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};

{{BASIS_DECLARATIONS | indent(4,True) }}

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
    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp() const override final;
    double getMaxTimeStamp() const override final;
    double getMinTimeStepSize() const override final;
    double getMaxTimeStepSize() const override final;

    /** Overwrites every time stamp the solver tracks; a checkpoint resume reinstates the stored time. */
    void setTimeStamp(double timeStamp);

    /**
     * Evaluates the refinement criterion over one cell patch.
     *
     * @param Q Whole nodal patch in AoS layout, or nullptr during grid construction.
     * @param x Patch centre.
     * @param h Patch size.
     * @param t Time.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double* const NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    /**
     * Sets the initial condition in one nodal point.
     *
     * @param x Position of the point within the computational domain.
     * @param h Size of the cell that holds the point.
     */
    virtual void initialCondition(
      double* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<none>" %}
    {% else %}
    /**
     * Applies the boundary conditions to one face point.
     *
     * The routine overwrites the inside and the outside values, and the
     * Riemann solver reads both afterwards.
     */
    virtual void boundaryConditions(
      const double* const NOALIAS Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double* const NOALIAS Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const double t,
      const int normal
    ) {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}

    void suspendSolversForOneGridSweep() override;

    /**
     * An override calls this base class operation.
     */
    void startGridConstructionStep() override;

    /**
     * An override calls this base class operation.
     */
    void finishGridConstructionStep() override;

    /**
     * An override calls this base class operation.
     */
    void startGridInitialisationStep() override;

    /**
     * An override calls this base class operation.
     */
    void finishGridInitialisationStep() override;

    /**
     * An override calls this base class operation.
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
     * An override calls this base class operation.
     */
    void finishTimeStep() override;

    /**
     * An override calls this base class operation.
     */
    void startPlottingStep() override;

    /**
     * An override calls this base class operation.
     */
    void finishPlottingStep() override;

    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;

    int getNumberOfDoFs() const override final;

    /**
     * Adds one cell's time step size, time stamp and cell size to the extrema
     * of the current traversal.
     */
    void update(
      const double timeStepSize,
      const double timeStamp,
      const double cellSize
    );

    /**
     * Reduces the extrema the current traversal has accumulated through update()
     * into the solver. Runs once per tree at traversal end, so the per-cell
     * update writes thread-local data only.
     */
    void reduceLocalUpdates();

    SolverState  getSolverState() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    /**
     * An override calls this base class operation.
     */
    void startSimulation() override;

    /**
     * An override calls this base class operation.
     */
    void finishSimulation() override;

    {% if STATELESS_PDE_TERMS %}
    /**
     * Returns whether the cell uses the static versions of the PDE operators,
     * which lets the cell run on the GPU. An override returns false for a cell
     * that needs the solver state.
     */
    {% else %}
    /**
     * Returns true, since the solver holds no stateless PDE terms. The
     * operation lets one code version serve a GPU-enabled solver and a solver
     * without GPU support.
     */
    {% endif %}
    virtual bool cellCanUseStatelessPDETerms(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellH,
      const double timeStamp,
      const double timeStepSize
    ) const;

  protected:
    /** Stores one traversal's extrema until reduceLocalUpdates() reduces them. */
    struct LocalUpdates {
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

    SolverState  _solverState;
    SolverState  _solverStateBeforeDynamicMeshRefinement;

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

    double _minTimeStepSize;
    double _maxTimeStepSize;

    double _minTimeStepSizeThisTimeStep;
    double _maxTimeStepSizeThisTimeStep;

  {{ABSTRACT_SOLVER_USER_DECLARATIONS}}
};
