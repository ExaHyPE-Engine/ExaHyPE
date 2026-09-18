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
#include "exahype2/dg/AMRMatrices.h"

#include "peano4/Constants.h"

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
   * Names the variable indices, so Q[Shortcuts::{{VARIABLE_NAMES[0]}}] reads the first variable.
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

    static std::string toString(SolverState state);

    static constexpr int DGOrder = {{DG_ORDER}};

    static constexpr int NumberOfUnknowns = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};

{{BASIS_DECLARATIONS | indent(4,True) }}

    static constexpr ::exahype2::dg::AMRMatrices1D<DGOrder + 1> InterpolationMatrix1D =
      ::exahype2::dg::interpolationMatrix1D<DGOrder + 1>(QuadraturePoints1D);
    static constexpr ::exahype2::dg::AMRMatrices1D<DGOrder + 1> RestrictionMatrix1D =
      ::exahype2::dg::restrictionMatrix1D<DGOrder + 1>(QuadraturePoints1D);

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

    /**
     * Evaluates the refinement criterion over one cell patch.
     *
     * @param Q Whole nodal patch in AoS layout, or nullptr during grid construction.
     * @param x Patch centre.
     * @param h Patch size.
     * @param t Time.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double* NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    /**
     * Sets the initial condition in one nodal point.
     *
     * @param x Position of the point within the computational domain.
     * @param h Size of the cell that holds the point.
     */
    virtual void initialCondition(
      double* NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
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
      const double* NOALIAS Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      double* NOALIAS Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double t,
      int normal
    ) {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}

    void suspendSolversForOneGridSweep() override;

    void startGridConstructionStep() override;
    void finishGridConstructionStep() override;

    void startGridInitialisationStep() override;
    void finishGridInitialisationStep() override;

    void startTimeStep() override;

    void startDynamicMeshRefinementStep() override;
    void finishDynamicMeshRefinementStep() override;

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
      double cellSize
    );

    void reduceLocalUpdates();

    SolverState  getSolverState() const;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    void startSimulation() override;
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
      double timeStamp,
      double timeStepSize
    ) const;

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
      bool                              empty{true};
    };

    static thread_local LocalUpdates _localUpdates;

    SolverState _solverState{SolverState::GridConstruction};
    SolverState _solverStateBeforeDynamicMeshRefinement{SolverState::GridConstruction};

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

    double _minTimeStepSize{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSize{0.0};

    double _minTimeStepSizeThisTimeStep{std::numeric_limits<double>::infinity()};
    double _maxTimeStepSizeThisTimeStep{0.0};

  {{ABSTRACT_SOLVER_USER_DECLARATIONS}}
};
