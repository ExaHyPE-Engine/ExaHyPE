// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FunctorTask.h"

tarch::tasking::FunctorTask::FunctorTask(std::function<void()> taskFunctor) :
  _taskFunctor(std::move(taskFunctor)) {}

void tarch::tasking::FunctorTask::run() { _taskFunctor(); }
