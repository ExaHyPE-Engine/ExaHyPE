// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_SOLVER_H_
#define _EXAHYPE2_SOLVER_H_



namespace exahype2 {
  class Solver;
}



class exahype2::Solver {
  public:
    virtual double getMinTimeStamp() const = 0;
    virtual double getMaxTimeStamp() const = 0;
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
