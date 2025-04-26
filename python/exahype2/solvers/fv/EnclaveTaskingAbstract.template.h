// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "exahype2/Solver.h"
#include "exahype2/RefinementControl.h"

#include "peano4/utils/Globals.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/accelerator.h"
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

  {% if USE_VARIABLE_SHORTCUT is sameas True %}
  /*
   * Enables the usage of shortcuts to access variables, e.g. use Q[s::{{VARIABLE_NAMES[0]}}] instead of Q[0]
   */
  using s = {{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}};
  {% endif %}
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public ::exahype2::Solver {
  public:
    enum class SolverState {
      GridConstruction,
      GridInitialisation,

      Primary,
      PrimaryAfterGridInitialisation,
      Secondary,

      Plotting,
      PlottingAfterGridInitialisation,

      Suspended
    };

    static std::string toString(SolverState);

    static constexpr int NumberOfFiniteVolumesPerAxisPerPatch = {{NUMBER_OF_VOLUMES_PER_AXIS}};

    /**
     * Maximum mesh size that this solver wants/is allowed to support. Peano 4
     * has to ensure that none of its ExaHyPE 2 solvers has to work on a mesh
     * which is coarser than its MaxH value.
     */
    static constexpr double MaxAdmissiblePatchH  = {{MAX_VOLUME_H}} * NumberOfFiniteVolumesPerAxisPerPatch;
    static constexpr double MaxAdmissibleVolumeH = {{MAX_VOLUME_H}};
    static constexpr double MaxAdmissibleCellH   = MaxAdmissiblePatchH;

    /**
     * Minimum mesh size that this solver wants to support. Peano 4 tries
     * to ensure that none of its ExaHyPE 2 solvers has to work on a mesh
     * which is finer than its MinH value. However, if there are multiple
     * solvers, the interplay of these solvers might imply that some solvers
     * work with fine mesh resolutions even though they have specified a
     * coarser MinH.
     */
    static constexpr double MinAdmissiblePatchH  = {{MIN_VOLUME_H}} * NumberOfFiniteVolumesPerAxisPerPatch;
    static constexpr double MinAdmissibleVolumeH = {{MIN_VOLUME_H}};
    static constexpr double MinAdmissibleCellH   = MinAdmissiblePatchH;

    {{CLASSNAME}}();

    /**
     * Alias for periodic boundary conditions.
     */
    static std::bitset<Dimensions> PeriodicBC;

    double getMinTimeStamp(bool ofCurrentlyRunningGridSweep=false) const final;
    double getMaxTimeStamp(bool ofCurrentlyRunningGridSweep=false) const final;
    double getMinTimeStepSize() const final;
    double getMaxTimeStepSize() const final;

    /**
     * Feel free to overwrite to bring in (dynamic) AMR.
     *
     * @param Q Vector of unknowns
     * @param t Time
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const {{STORAGE_PRECISION}}* NOALIAS    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t
    ) {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    virtual void initialCondition(
      {{STORAGE_PRECISION}}* NOALIAS          Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      bool                                         gridIsConstructed
    ) {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}

    /**
     * Apply boundary conditions. You can overwrite both the inside and
     * outside values though most BCs only modify the outside ones. Please
     * note that the boundary conditions you set here are after that subject
     * to the Riemann solver, i.e. flux and eigenvalues.
     */
    virtual void boundaryConditions(
      const {{STORAGE_PRECISION}}* NOALIAS    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{STORAGE_PRECISION}}* NOALIAS          Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      int                                          normal
    ) {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}

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
    virtual void startTimeStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void finishTimeStep() override;

    virtual void printTimeStep() override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void startPlottingStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) override;

    /**
     * If you hook into this routine, ensure the abstract base class
     * operation is still invoked.
     */
    virtual void finishPlottingStep() override;

    virtual void suspendSolversForOneGridSweep() override;

    /**
     * @param currentTimeStep If you set this to false, you'll get the
     *   quantity from the preceding time step. This is important for
     *   local time stepping with fixed subcycling, as they have to
     *   know the sizes from the last time step throughout the traversal,
     *   where the current patch size might just be re-evaluated.
     *
     * @return Actually observed sizes, not the admissible quantities
     */
    double getMaxPatchSize(bool ofCurrentlyRunningGridSweep = true) const;
    double getMinPatchSize(bool ofCurrentlyRunningGridSweep = true) const;
    double getMaxVolumeSize(bool ofCurrentlyRunningGridSweep = true) const;
    double getMinVolumeSize(bool ofCurrentlyRunningGridSweep = true) const;

    /**
     * mesh is an aAlias for patch
     */
    virtual double getMaxMeshSize() const override final;
    virtual double getMinMeshSize() const override final;

    /**
     * Update the global solver state, i.e. inform the solver about some
     * updated global quantities.
     *
     * @see setTimeStepSize(double)
     */
    void update(double timeStepSize, double timeStamp, double patchSize);

    SolverState getSolverState() const;

    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static constexpr int NumberOfUnknowns           = {{NUMBER_OF_UNKNOWNS}};
    static constexpr int NumberOfAuxiliaryVariables = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    {{SOLVER_CONSTANTS}}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif

    /**
     * It is important that we only plot after the secondary sweep.
     * All the switching happens in startTimeStep(), so it is fine if
     * we test against secondary. If an outer routine (the main) calls
     * this function, this means that the secondary traversal just has
     * terminated.
     */
    virtual bool mayPlot() const override;

    /**
     * As we work with enclave tasking, we have to run multiple grid sweeps
     * per time step. With this routine, you can find out if the current
     * sweep (in beginIteration(), e.g.) is the first one.
     *
     * Throughout the traversal, this field is correct. If you query it in
     * startTimeStep(), you have to be aware that the abstract solver's
     * implementations switches the state. After you've called the abstract
     * solver's implementation, the predicate is correct. Before, you have
     * to negate it.
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

  protected:
    static tarch::logging::Log  _log;

    SolverState  _solverState;

    double     _minTimeStamp;
    double     _maxTimeStamp;

    double     _minTimeStampThisTimeStep;
    double     _maxTimeStampThisTimeStep;

    double     _localMinTimeStampThisTimeStep;
    double     _localMaxTimeStampThisTimeStep;

    double     _minVolumeH;
    double     _maxVolumeH;

    double     _minVolumeHThisTimeStep;
    double     _maxVolumeHThisTimeStep;

    double     _minTimeStepSize;
    double     _maxTimeStepSize;

    double     _minTimeStepSizeThisTimeStep;
    double     _maxTimeStepSizeThisTimeStep;

    tarch::multicore::BooleanSemaphore  _semaphore;

  {{ABSTRACT_SOLVER_USER_DECLARATIONS}}
};

{# Empty line here #}
