#include "tarch/Assertions.h"
#include "../Tasks.h"
#include "tarch/multicore/multicore.h"


#ifdef SharedCPP
#include <thread>
#include <queue>
#include <list>
#include <mutex>
#include <atomic>

#include "CPP.h"
#include "tarch/logging/Log.h"


/**
 * This class is a 1:1 mirror of my TBB implementation. Please see
 * documentation there.
 */
namespace {
  std::list<tarch::multicore::Task* > nonblockingTasks;
  std::mutex                           taskQueueMutex;
}



void tarch::multicore::yield() {
  std::this_thread::yield();
}


bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);
  bool result = false;
  while ( maxTasks>0 ) {
	// try to get a task and store in p
    Task* myTask = nullptr;
    std::vector<Task*> work;
    taskQueueMutex.lock();

    if (nonblockingTasks.empty()) {
     maxTasks = 0;
    }
    else
    {
       while (not nonblockingTasks.empty() and (work.size() < std::min(maxTasks, int(nonblockingTasks.size())) ) )
       {

         myTask = nonblockingTasks.front();
         nonblockingTasks.pop_front();
         work.push_back(myTask);
       }
    }
    taskQueueMutex.unlock();

    //std::cerr << std::this_thread::get_id() << " will process " << work.size() << " tasks while maxTasks is " << maxTasks <<"\n";

    if (myTask!=nullptr)
    {
       while (work.size()>0)
       {
          myTask = work.back();
          bool requeue = myTask->run();
          if (requeue) {
            spawnTask( myTask );
          }
          else {
            delete myTask;
          }
          work.pop_back();
          maxTasks--;
          result = true;

       }
    }

  }
  return result;
}

//namespace {
  //std::atomic<int> numberOfConsumerTasks(0);


  //void spawnConsumerTask( int numberOfTasks = tarch::multicore::getNumberOfPendingTasks() ) {
    //numberOfConsumerTasks++;

    //std::thread([numberOfTasks]() {
      //bool hasProcessedTasks = tarch::multicore::processPendingTasks( numberOfTasks );
      //int  newNumberOfTasks  = tarch::multicore::getNumberOfPendingTasks();

      //if (hasProcessedTasks and newNumberOfTasks>numberOfTasks*2) {
        //spawnConsumerTask( numberOfTasks );
        //spawnConsumerTask( numberOfTasks );
      //}
      //else if (hasProcessedTasks and newNumberOfTasks>numberOfTasks) {
        //spawnConsumerTask( numberOfTasks+1 );
      //}
      //else if (hasProcessedTasks and numberOfTasks>1) {
        //spawnConsumerTask( numberOfTasks-1 );
      //}
      //numberOfConsumerTasks--;
    //}).detach();
  //}
//}


void tarch::multicore::spawnTask(Task*  job) {
  taskQueueMutex.lock();
  nonblockingTasks.push_back(job);
  //std::cerr << "SPAWN NBQ size now: " << nonblockingTasks.size() <<  " consumer: " << numberOfConsumerTasks << "\n";
  taskQueueMutex.unlock();
}


void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  std::atomic<int> counter(tasks.size());
  std::thread t[ tasks.size() ];
  int max_tasks = 50;
  // NOTE: this is likely not performant
  if (std::getenv("MAXTASKS") != nullptr) max_tasks = atoi(std::getenv("MAXTASKS"));

  // Launch threads
  for (int i = 0; i < tasks.size(); ++i) {
    t[i] = std::thread( [&,i]()->void {
      while (tasks[i]->run()) {}
      //std::cerr << std::this_thread::get_id() << " ran\n";
      delete tasks[i];
      counter--;

      // This is the backfilling part
      while (counter>0 && not nonblockingTasks.empty())
      {
        //std::cerr << std::this_thread::get_id() << " sees " << nonblockingTasks.size() << " NBTs and counter " << counter << "\n";
        tarch::multicore::processPendingTasks(max_tasks);
      }
    });
  }

  for (int i = 0; i < tasks.size(); ++i) {
	t[i].join();
  }
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size();
}

bool tarch::multicore::processTask(int number) {

  // Iterate backwards through list
  Task* myTask = nullptr;
  taskQueueMutex.lock();
  for (auto it = nonblockingTasks.end(); it !=nonblockingTasks.begin();)
  {
    --it;
    {
       myTask = (*it);
       //std::cout << myTask->getTaskId() << "/" << nonblockingTasks.size() << " I want " << number << "\n";
       nonblockingTasks.erase(it);
       it=nonblockingTasks.begin();
    }
  }

  taskQueueMutex.unlock();

  if (myTask != nullptr)
  {
    while (myTask->run()) {};
    delete myTask;
    return true;
  }
  else
  {
    return false;
  }
}

void tarch::multicore::cpp::shutdownConsumerTasks() {
  //static tarch::logging::Log _log( "tarch::multicore::cpp" );
  //logTraceInWith1Argument( "shutdownConsumerTasks()", numberOfConsumerTasks.fetch_add(0) );
  //while (numberOfConsumerTasks.fetch_add(0)>0) {
	//yield();
  //}
  //logTraceOut( "shutdownConsumerTasks()" );
}


#endif
