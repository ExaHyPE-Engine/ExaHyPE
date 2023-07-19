// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



namespace exahype2 {
  class Solver;
}



class exahype2::Solver {
  public:
    /**
     * This is a "fake" enum, i.e. we do not use it to distinguish different
     * variants. Instead, we use it as a fix that allows us to "overload"
     * operations:
     *
     * In C++ you cannot overload w.r.t. static. We however need functions which
     * exist twice in ExaHyPE: Once as standard (virtual) member functions and
     * once at static version which an be offloaded to a GPU as it does not
     * have a state. Both function variants, in theory, have the same signature
     * but if they had, a compiler could not distinguish them. So I use this
     * enum for the GPU version.
     *
     * If you create a solver without GPU support, this enum will not be used.
     * It is however always created. Once you write a GPU version and then compile
     * without GPU support, you will thus still be able to have all your GPU
     * function variants, and you don't have to work with ifdefs.
     */
    enum class Offloadable {
      Yes
    };

    /**
     * There are two different falvours of a minimal time stamp: On the one hand,
     * there's a global minimum time stamp over the whole mesh. This might not
     * be the min time stamp after the last update. If you have local time
     * stepping, then some cells might just have done a tiny time step,
     * whereas the big cells still span a large time span. Hence, no the other
     * hand, there's also a (time-)local time stamp.
     */
    virtual double getMinTimeStamp(bool ofLastTimeStepOnly=false) const = 0;
    virtual double getMaxTimeStamp(bool ofLastTimeStepOnly=false) const = 0;
    virtual double getMinTimeStepSize() const = 0;
    virtual double getMaxTimeStepSize() const = 0;

    virtual void startGridConstructionStep() = 0;
    virtual void finishGridConstructionStep() = 0;

    virtual void suspendSolversForOneGridSweep() = 0;

    virtual void startGridInitialisationStep() = 0;
    virtual void finishGridInitialisationStep() = 0;

    virtual void startTimeStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) = 0;
    virtual void finishTimeStep() = 0;

    virtual void startPlottingStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) = 0;
    virtual void finishPlottingStep() = 0;

    virtual double getMaxMeshSize() const = 0;
    virtual double getMinMeshSize() const = 0;

    /**
     * Not all solvers allow you to plot after each grid sweep. If a
     * solver needs multiple steps, it might want to veto that you
     * plot intermediate data.
     */
    virtual bool mayPlot() const = 0;

    virtual void startSimulation() = 0;
    virtual void finishSimulation() = 0;
};


