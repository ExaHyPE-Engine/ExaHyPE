// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "TPSCalculator.h"


tarch::timing::TPSCalculator::TPSCalculator():
  _currentTPS(0.0),
  _startTimeStep(0),
  _currentTimeStep(0) {}


void tarch::timing::TPSCalculator::update(double elapsedWalltime) {
  _startTimeStep = _currentTimeStep;
  _currentTPS    = double(++_currentTimeStep - _startTimeStep) / elapsedWalltime;
}


double tarch::timing::TPSCalculator::getTPS() const { return _currentTPS; }
