// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_SOLVER_H_
#define _EXAHYPE2_SOLVER_H_



namespace exahype2 {
  class Solver;
}



class exahype2::Solver {
  public:
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
};


#endif
