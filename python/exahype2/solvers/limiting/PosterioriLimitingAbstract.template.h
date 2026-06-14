// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "exahype2/Solver.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"
#include "tarch/instrumentation/Log.h"
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
      RegularSolver,
      LimiterStatusSpreadingToNeighbours,
      LimiterStatusSpreadingToSecondNeighbours,
      LimiterSolver,
      Plotting,
      Suspended
    };

    static std::string toString(SolverState);

    {{CLASSNAME}}();
    ~{{CLASSNAME}}();

    SolverState getSolverState() const;

    double getMinTimeStamp() const final;
    double getMaxTimeStamp() const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;

    virtual double getMaxPatchSize() const override final;
    virtual double getMinPatchSize() const override final;
    virtual double getMaxCellSize() const override final;
    virtual double getMinCellSize() const override final;

    virtual int getNumberOfDoFs() const override final;

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
        static constexpr int NumberOfDMPObservables                 = {{NUMBER_OF_DMP_OBSERVABLES}};
        static constexpr {{COMPUTE_PRECISION}} RelaxationParameter  = {{DMP_RELAXATION_PARAMETER}};
        static constexpr {{COMPUTE_PRECISION}} DifferencesScaling   = {{DMP_DIFFERENCES_SCALING}};

        /**
        * Inform the solver about a cell being troubled, meaning its value
        * needs to be recomputed and the solver cannot move on to the next
        * timestep until this is done.
        */
        void addTroubledCell();

        int getNumberOfTroubledCells() const;

        /**
        * Default implementation. Please overwrite.
        *
        * See superclass for function's semantics.
        */
        template<typename T>
        void mapDiscreteMaximumPrincipleObservables({{COMPUTE_PRECISION}}* const observables, const T* const Q) const {
          if (NumberOfDMPObservables>0) {
            for(int i=0; i<NumberOfDMPObservables; i++){
              observables[i] = Q[i];
            }
          }
        }

        /**
        * Default implementation. Please overwrite.
        *
        * See superclass for function's semantics.
        */
       {{COMPUTE_PRECISION}} getDiscreteMaximumPrincipleRelaxationParameter(
            const {{COMPUTE_PRECISION}}&  specifiedRelaxationParameter,
            const int&                    observable,
            const {{COMPUTE_PRECISION}}&  localMin,
            const {{COMPUTE_PRECISION}}&  localMax,
            const {{COMPUTE_PRECISION}}&  boundaryMinPerObservable,
            const {{COMPUTE_PRECISION}}&  previousMax) const {
          return specifiedRelaxationParameter;
        }

{% if ADMISSIBILITY_IMPLEMENTATION!="<none>" %}
    virtual bool isPhysicallyAdmissible(
        const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const                         Q,
        const tarch::la::Vector<DIMENSIONS,double>& x,
        const tarch::la::Vector<DIMENSIONS,double>& h,
        const double timeStamp) {% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %} = 0 {% else %} final {% endif %};
{% endif %}

  protected:

    SolverState _solverState;

    int _numberOfTroubledCells;
    int _numberOfTroubledCellsThisTimeStep;

    tarch::multicore::BooleanSemaphore _semaphore;

  {{SOLVER_USER_DECLARATIONS}}
};
