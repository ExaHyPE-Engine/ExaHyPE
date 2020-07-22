#include "tarch/Assertions.h"


#include <thread>
#include <queue>
#include "Tasks.h"
#include "multicore.h"


bool operator<( const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs ) {
  return lhs.getPriority() < rhs.getPriority();
}


bool tarch::multicore::TaskComparison::operator() (const Task& lhs, const Task& rhs) const {
  return lhs < rhs;
}


bool tarch::multicore::TaskComparison::operator() (Task* lhs, Task* rhs) const {
  return *lhs < *rhs;
}


tarch::multicore::Task::Task( int priority ):
  _priority( priority ) {
}


int tarch::multicore::Task::getPriority() const {
  return _priority;
}


void tarch::multicore::Task::setPriority( int priority ) {
  _priority = priority;
}


void tarch::multicore::Task::prefetch() {
}




tarch::multicore::TaskWithCopyOfFunctor::TaskWithCopyOfFunctor( const std::function<bool()>& taskFunctor ):
  _taskFunctor(taskFunctor)  {
}


bool tarch::multicore::TaskWithCopyOfFunctor::run() {
  return _taskFunctor();
}


tarch::multicore::TaskWithoutCopyOfFunctor::TaskWithoutCopyOfFunctor( std::function<bool()>& taskFunctor ):
  _taskFunctor(taskFunctor)  {
}


bool tarch::multicore::TaskWithoutCopyOfFunctor::run() {
  return _taskFunctor();
}


#ifndef SharedMemoryParallelisation

#include <thread>

namespace {
  std::queue<tarch::multicore::Task* > nonblockingTasks;
}


bool tarch::multicore::processPendingTasks(int maxTasks) {
  // Note: Only invoked if no shared memory parallelisation activated. If
  // TBB/C++/OpenMP are enabled, the routine of the respective subfolder is
  // invoked
  if (nonblockingTasks.empty()) {
    return false;
  }
  else {
    while ( maxTasks>0 and !nonblockingTasks.empty() ) {
      Task* p = nonblockingTasks.front();
      nonblockingTasks.pop();
      while (p->run()) {};
      delete p;
      maxTasks--;
    }
    return true;
  }
}


void tarch::multicore::spawnTask(Task*  job) {
  nonblockingTasks.push(job);
}


void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  for (auto& p: tasks) {
    while (p->run()) {}
    delete p;
  }
}


/**
 * This has to be the standard C++ implementation. Even if multithreading is
 * disabled, the code might logically run with multiple threads, and then we
 * have to switch the thread context from time to time.
 */
void tarch::multicore::yield() {
  std::this_thread::yield();
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

#endif
