// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "exahype2/Solver.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/NonCriticalAssertions.h"

#include "peano4/Globals.h"

#include "Constants.h"

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
      Suspended,
      Plotting {% for item in range(RK_STEPS) -%}
      ,
      ProjectOnFacesAndComputeVolumeIntegralOfStep{{item}},
      SolveRiemannProblemAndAddToVolumeIntegralOfStep{{item}}
      {%- endfor %}
    };

    static std::string toString(SolverState);

    static constexpr int  DGOrder = {{DG_ORDER}};

    /**
     * Maximum mesh size that this solver wants/is allowed to support. Peano 4
     * has to ensure that none of its ExaHyPE 2 solvers has to work on a mesh
     * which is coarser than its MaxH value.
     */
    static constexpr double MaxAdmissibleCellH  = {{MAX_CELL_H}};

    static constexpr int    NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int    NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};

    /**
     * Minimum mesh size that this solver wants to support. Peano 4 tries
     * to ensure that none of its ExaHyPE 2 solvers has to work on a mesh
     * which is finer than its MinH value. However, if there are multiple
     * solvers, the interplay of these solvers might imply that some solvers
     * work with fine mesh resolutions even though they have specified a
     * coarser MinH.
     */
    static constexpr double MinAdmissibleCellH  = {{MIN_CELL_H}};

{{BASIS_DECLARATIONS | indent(4,True) }}

    {{SOLVER_CONSTANTS}}

    {{CLASSNAME}}();

    /**
     * Alias for periodic boundary conditions.
     */
    static std::bitset<DIMENSIONS> PeriodicBC;

    double getMinTimeStamp() const final;
    double getMaxTimeStamp() const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;

    /**
     * Evaluate refinement criterion over cell
     *
     * @param Q Vector of unknowns
     * @param x Centre of underlying cell
     * @param h Size of underlying cell
     * @param t Time
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double * NOALIAS                  Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&  volumeCentre,
      const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
      double                                       t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    /**
     *
     * @param x     Position of point that is to be initialised within computational domain.
     * @param h     Size of cell within which the point is placed.
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
     * outside values though most BCs only modify the outside ones. Please
     * note that the boundary conditions you set here are after that subject
     * to the Riemann solver, i.e. flux and eigenvalues.
     */
    virtual void boundaryConditions(
      const double * NOALIAS                  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double * NOALIAS                        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&  x,
      double                                       t,
      int                                          normal
    ) {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}


    virtual void suspendSolversForOneGridSweep() override;

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

    virtual void logTimeStep() override;

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

    double getMaxCellSize() const;
    double getMinCellSize() const;

    /**
     * Within the DG context, mesh is an alias for cell.
     */
    virtual double getMaxMeshSize() const override final;
    virtual double getMinMeshSize() const override final;

    virtual double getMinAdmissibleMeshSize() const override final;
    virtual double getMaxAdmissibleMeshSize() const override final;

    virtual int getNumberOfDoFs() const override final;

    /**
     * Update the global solver state, i.e. inform the solver about some
     * updated global quantities.
     *
     * @see setTimeStepSize(double)
     */
    void update(double timeStepSize, double timeStamp, double cellSize);

    SolverState  getSolverState() const;

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

    /**
     * Feel free to overwrite in user code, but ensure the superclass
     * implementation is still invoked, too.
     */
    virtual void finishSimulation() override;

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
    static tarch::logging::Log  _log;

    SolverState  _solverState;

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

    int        _cellUpdates;

    tarch::multicore::BooleanSemaphore  _semaphore;

  {{ABSTRACT_SOLVER_USER_DECLARATIONS}}
};
