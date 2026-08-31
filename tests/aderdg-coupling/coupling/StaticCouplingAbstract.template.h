// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Solver.h"

#include "tarch/la/Vector.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"

#include "peano4/Globals.h"

#include "Constants.h"

{{SOLVER_INCLUDES}}

namespace {{NAMESPACE | join("::")}} {
  class {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} : public ::exahype2::Solver {
  public:
    /**
     * A member function and its offloadable counterpart carry the same
     * signature, so the offloadable one takes an argument of this type to
     * distinguish the two.
     */
    enum class Offloadable {
      Yes
    };

    enum class SolverState {
      GridConstruction,
      GridInitialisation,
      LimiterStatusSpreadingToNeighbours,
      TimeStepping,
      Plotting,
      PlottingAfterGridInitialisation,
      Suspended
    };

    static std::string toString(const SolverState state);

    {{CLASSNAME}}();
    ~{{CLASSNAME}}();

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
     * A subclass that overrides one of the start or finish routines below calls
     * this class's implementation, as they maintain _solverState and reduce the
     * counter of troubled cells.
     */
    void startGridConstructionStep() override;
    void finishGridConstructionStep() override;

    void startGridInitialisationStep() override;
    void finishGridInitialisationStep() override;

    void suspendSolversForOneGridSweep() override;

    void startTimeStep() override;
    void finishTimeStep() override;

    void startPlottingStep() override;
    void finishPlottingStep() override;

    bool mayPlot() const override;

    bool isFirstGridSweepOfTimeStep() const;
    bool isLastGridSweepOfTimeStep() const;

    void startSimulation() override;
    void finishSimulation() override;

    /**
     * Adds one to the counter of troubled cells. The solver recomputes such a
     * cell before it moves on to the next time step.
     */
    void addTroubledCell();
    int getNumberOfTroubledCells() const;

    /**
     * Exists so the generated kernels compile. A call fails an assertion.
     */
    void mapDiscreteMaximumPrincipleObservables(double* const observables, const double* const Q) const {
      assertion(false, "Discrete maximum principle observables are not mapped by this solver.");
    }

    /**
     * Exists so the generated kernels compile. A call fails an assertion.
     */
    double getDiscreteMaximumPrincipleRelaxationParameter(
      const double& specifiedRelaxationParameter,
      const int&    observable,
      const double& localMin,
      const double& localMax,
      const double& boundaryMinPerObservable,
      const double& previousMax
    ) const {
      assertion(false, "The discrete maximum principle relaxation parameter is not defined by this solver.");
      return 0.0;
    }

{% if ADMISSIBILITY_IMPLEMENTATION!="<none>" %}
    virtual bool isPhysicallyAdmissible(
        const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const                         Q,
        const tarch::la::Vector<DIMENSIONS,double>& x,
        const tarch::la::Vector<DIMENSIONS,double>& h,
        const double timeStamp) {% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %} = 0 {% else %} final {% endif %};
{% endif %}

  protected:
    SolverState _solverState{SolverState::GridConstruction};

    int _totalNumberOfTroubledCells{0};

  {{SOLVER_USER_DECLARATIONS}}
};
