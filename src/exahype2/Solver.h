// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2 {
  class Solver;
} // namespace exahype2

class exahype2::Solver {
public:
  /**
   * This is a "fake" enum, i.e. we do not use it to distinguish different
   * variants. Instead, we use it as a fix that allows us to "overload"
   * operations:
   *
   * In C++ you cannot overload w.r.t. static. We however need functions which
   * exist twice in ExaHyPE 2: Once as standard (virtual) member functions and
   * once as a static version which can be offloaded to a GPU as it does not
   * have a state. Both function variants, in theory, have the same signature
   * but if they had, a compiler could not distinguish them. This enum marks
   * the GPU version.
   *
   * If you create a solver without GPU support, this enum will not be used.
   * It is however always created. Once you write a GPU version and then compile
   * without GPU support, you will thus still be able to have all your GPU
   * function variants, and you don't have to work with ifdefs.
   */
  enum class Offloadable { Yes };

  Solver()          = default;
  virtual ~Solver() = default;

  // Solvers live as one instance per repository and are always handled through this base,
  // so copying one would slice it.
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;

  virtual double getMinTimeStamp() const    = 0;
  virtual double getMaxTimeStamp() const    = 0;
  virtual double getMinTimeStepSize() const = 0;
  virtual double getMaxTimeStepSize() const = 0;

  virtual void startGridConstructionStep()  = 0;
  virtual void finishGridConstructionStep() = 0;

  virtual void suspendSolversForOneGridSweep() = 0;

  virtual void startGridInitialisationStep()  = 0;
  virtual void finishGridInitialisationStep() = 0;

  virtual void startTimeStep()  = 0;
  virtual void finishTimeStep() = 0;

  /**
   * This pair brackets every sweep of a dynamic mesh refinement step. The base
   * implementation records that mesh refinement ran, so a solver's next
   * startTimeStep() can react to a changed grid and reset the flag. A solver
   * that keeps further state per refinement step overrides these operations
   * and calls the base version.
   */
  virtual void startDynamicMeshRefinementStep() { _meshRefinementRanSinceLastTimeStep = true; }
  virtual void finishDynamicMeshRefinementStep() {}

  virtual void startPlottingStep()  = 0;
  virtual void finishPlottingStep() = 0;

  virtual double getMaxPatchSize() const = 0;
  virtual double getMinPatchSize() const = 0;
  virtual double getMaxCellSize() const  = 0;
  virtual double getMinCellSize() const  = 0;

  virtual int getNumberOfDoFs() const = 0;

  /**
   * Not all solvers allow you to plot after each grid sweep. If a
   * solver needs multiple steps, it might want to veto that you
   * plot intermediate data.
   */
  virtual bool mayPlot() const = 0;

  virtual void startSimulation()  = 0;
  virtual void finishSimulation() = 0;

  /**
   * Has a completed sweep stored face data yet?
   *
   * The boundary exchange consumes each sweep's sent batch at the start of the
   * following sweep, so a sweep may only receive once an earlier sweep has sent.
   * A guard keyed on the solver state alone cannot express that, because the
   * state stays at initialisation across the whole initialise-drain loop whose
   * every sweep sends. Initialisation sets the flag once its first storing sweep
   * has finished.
   */
  bool hasStoredFaceData() const { return _hasStoredFaceData; }

protected:
  bool _meshRefinementRanSinceLastTimeStep = false;
  bool _hasStoredFaceData                  = false;
};
