// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENCLAVE_BOOKKEEPING_H_
#define _EXAHYPE2_ENCLAVE_BOOKKEEPING_H_


#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"


#include <set>
#include <unordered_map>
#include <string>


namespace exahype2 {
  class EnclaveBookkeeping;
  class EnclaveTask;
}


/**
@todo Das Ergebnis muss lokal gepuffert werden im bookkeeping.
      Ich brauche zwei Adapter: einen fuer primaer, einen fuer sekundaer. Sonst rekonstruier ich dei Loesung jedes Mal (ist teuer)
      Die Adapter muessen alle das gleiche Bookkeeping verwenden -> singleton
      Ein Task muss sich, sobald er zuende ist, in den Bookkeeping einreihen, d.h. hier gibt es zwei Datenstrukturen: Semaphoren (set) + Tasks (map)
 */
class exahype2::EnclaveBookkeeping {
  private:
    static tarch::logging::Log          _log;

    const static std::string MemoryAllocationsInLookupTableIdentifier;
    const static std::string LookupMissesIdentifier;

    tarch::multicore::BooleanSemaphore  _activeTasksSemaphore;
    tarch::multicore::BooleanSemaphore  _finishedTasksSemaphore;
    std::set<int>                       _activeTaskNumbers;

    /**
     * Plain map onto ouput array. See lifecycle discussion of EnclaveTask
     * for details.
     */
    std::unordered_map<int, std::pair<int,double*> >       _finishedTasks;

    EnclaveBookkeeping() = default;
  public:
    static constexpr int NoEnclaveTaskNumber = -1;

    static EnclaveBookkeeping& getInstance();

    /**
     * For debugging only. This routine is not thread-safe.
     */
    void dumpStatistics();

    void waitForTaskToTerminateAndCopyResultOver(int number, double* destination);

    /**
     * Usually called directly by EnclaveTask.
     *
     * This routine has to return a number which is currently not in use.
     * There is however no need for the delivered task numbers to be
     * consecutive. Therefore, I use the size of the existing task set as
     * a guideline for a task number to search for. If this initial guess
     * is already handed out, I increase the counter by a prime number and
     * try the new number again - until I've finally found a task which is
     * not yet processed.
     */
    int reserveTaskNumber();

    /**
     * Usually called directly by EnclaveTask.
     *
     * Once this routine is called, the ownership of data goes
     *
     * @param data Has to be device memory allocated through tarch::multicore::allocateMemory()
     */
    void finishedTask(int taskNumber, int numberOfResultValues, double* data);
};

#endif
