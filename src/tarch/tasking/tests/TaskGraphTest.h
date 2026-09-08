// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tasking::tests {
  class TaskGraphTest;
} // namespace tarch::tasking::tests

class tarch::tasking::tests::TaskGraphTest final : public tarch::tests::TestCase {
public:
  TaskGraphTest();
  TaskGraphTest(const TaskGraphTest&)            = delete;
  TaskGraphTest(TaskGraphTest&&)                 = delete;
  TaskGraphTest& operator=(const TaskGraphTest&) = delete;
  TaskGraphTest& operator=(TaskGraphTest&&)      = delete;
  ~TaskGraphTest() override                      = default;

  void run() override;

private:
  void testSpawnIndependentTask();
  void testSpawnFunctorTask();
  void testSpawnTaskWithOutDependency();
  void testSpawnTaskWithInAndOutDependency();
  void testSpawnTaskWithMultipleInAndOutDependencies();
  void testSpawnTaskIgnoresNullAndDuplicateDependencies();
  void testSpawnTaskWithManyOutDependencies();
  void testSpawnTaskWithManyInDependencies();
  void testSpawnTaskWithManyInAndOutDependencies();
  void testWaitForDependencies();
  void testRepeatedSweepsReuseTheSameTokens();
  void testTasksMaySpawnDependentTasks();
};
