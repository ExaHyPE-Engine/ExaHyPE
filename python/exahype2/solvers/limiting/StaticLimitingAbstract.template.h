// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "exahype2/Solver.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include "peano4/utils/Globals.h"

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
    ~{{CLASSNAME}}();

    SolverState getSolverState() const;

    double getMinTimeStamp(bool ofCurrentlyRunningGridSweep=false) const final;
    double getMaxTimeStamp(bool ofCurrentlyRunningGridSweep=false) const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;

    virtual double getMaxMeshSize() const override final;
    virtual double getMinMeshSize() const override final;

    //TODO: link these to the max and min of the other solvers
    static constexpr double MaxAdmissibleCellH  = 1000.;
    static constexpr double MinAdmissibleCellH  = 0.;

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

    virtual void suspendSolversForOneGridSweep() override;

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

    public:
        /**
        * Inform the solver about a cell being troubled, meaning its value
        * needs to be recomputed and the solver cannot move on to the next
        * timestep until this is done.
        */
        void addTroubledCell();

        int getNumberOfTroubledCells() const;

        /**
        * Only exists for the generated kernels, should never be called
        */
       template<typename T>
        void mapDiscreteMaximumPrincipleObservables({{COMPUTE_PRECISION}}* const observables, const T* const Q) const {
          ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "map discrete maximum principle observables was called, it should never be.", "" );
        }

        /**
        * Only exists for the generated kernels, should never be called
        */
        {{COMPUTE_PRECISION}} getDiscreteMaximumPrincipleRelaxationParameter(
          const {{COMPUTE_PRECISION}}&  specifiedRelaxationParameter,
          const int&                    observable,
          const {{COMPUTE_PRECISION}}&  localMin,
          const {{COMPUTE_PRECISION}}&  localMax,
          const {{COMPUTE_PRECISION}}&  boundaryMinPerObservable,
          const {{COMPUTE_PRECISION}}&  previousMax) const {
            ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "get discrete maximum principle relaxation parameter was called, it should never be.", "" );
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
    static tarch::logging::Log _log;

    SolverState _solverState;

    int _numberOfTroubledCells;

    tarch::multicore::BooleanSemaphore _semaphore;

  {{SOLVER_USER_DECLARATIONS}}
};
