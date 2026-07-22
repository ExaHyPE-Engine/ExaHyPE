// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "exahype2/Solver.h"

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include "peano4/Globals.h"

#include "kernels/{{SOLVER_NAME}}/Quadrature.h"

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
    enum class SolverState {
      GridConstruction,
      GridInitialisation,
      LimiterStatusSpreadingToNeighbours,
      TimeStep,
      Plotting,
      PlottingAfterGridInitialisation,
      Suspended
    };

    static std::string toString(SolverState);

    {{CLASSNAME}}();
    ~{{CLASSNAME}}() override;

    {{CLASSNAME}}(const {{CLASSNAME}}&)            = delete;
    {{CLASSNAME}}({{CLASSNAME}}&&)                 = delete;
    {{CLASSNAME}}& operator=(const {{CLASSNAME}}&) = delete;
    {{CLASSNAME}}& operator=({{CLASSNAME}}&&)      = delete;

    SolverState getSolverState() const;

    double getMinTimeStamp() const override final;
    double getMaxTimeStamp() const override final;
    double getMinTimeStepSize() const override final;
    double getMaxTimeStepSize() const override final;

    double getMaxPatchSize() const override final;
    double getMinPatchSize() const override final;
    double getMaxCellSize() const override final;
    double getMinCellSize() const override final;

    int getNumberOfDoFs() const override final;

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

    void suspendSolversForOneGridSweep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    void startTimeStep() override;

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

    /**
     * Inform the solver about a cell being troubled, meaning its value
     * needs to be recomputed and the solver cannot move on to the next
     * time step until this is done.
     */
    void addTroubledCell();

    int getNumberOfTroubledCells() const;

    /**
     * Exists only to satisfy the generated kernels and must never be called.
     */
    template<typename T>
    void mapDiscreteMaximumPrincipleObservables({{COMPUTE_PRECISION}}* const observables, const T* const Q) const {
      assertion(false, "Routine mapDiscreteMaximumPrincipleObservables must never be called.");
    }

    /**
     * Exists only to satisfy the generated kernels and must never be called.
     */
    {{COMPUTE_PRECISION}} getDiscreteMaximumPrincipleRelaxationParameter(
      const {{COMPUTE_PRECISION}}&  specifiedRelaxationParameter,
      const int&                    observable,
      const {{COMPUTE_PRECISION}}&  localMin,
      const {{COMPUTE_PRECISION}}&  localMax,
      const {{COMPUTE_PRECISION}}&  boundaryMinPerObservable,
      const {{COMPUTE_PRECISION}}&  previousMax) const {
      assertion(false, "Routine getDiscreteMaximumPrincipleRelaxationParameter must never be called.");
      return 0.0;
    }

{% if ADMISSIBILITY_IMPLEMENTATION!="<none>" %}
    virtual bool isPhysicallyAdmissible(
        const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const Q,
        const tarch::la::Vector<DIMENSIONS,double>&       x,
        const tarch::la::Vector<DIMENSIONS,double>&       h,
        const double timeStamp) {% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %} = 0 {% else %} final {% endif %};
{% endif %}

  protected:
    SolverState _solverState;

    int _numberOfTroubledCells;

    tarch::multicore::BooleanSemaphore _semaphore;

  {{SOLVER_USER_DECLARATIONS}}
};
