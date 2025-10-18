// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "tarch/multicore/Task.h"

#include <functional>

namespace exahype2 {
  template <typename T, int>
  class EnclaveTask;
} // namespace exahype2

/**
 * Base class for all enclave tasks
 *
 * Enclave tasks per se are application-generic. They are given an array of
 * values, they are told how big the output data has to be, and they are also
 * given a functor which tells them what to run on the outcome. They don't know
 * anything about the functor's semantics.
 *
 *
 * ## Usage:
 *
 * Every ExaHyPE solver category defines its own enclave tasks being subclasses
 * of the present, generic type. Enclave tasks may be spawned with
 * data, so we can efficiently wait for them.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * auto newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
 *   ...
 * );
 *
 * ...
 *
 * tarch::multicore::spawnTask(newEnclaveTask);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *
 * ## Data ownership
 *
 * The enclave task does not allocate or free any own data. It purely works on the given
 * input and output data pointers.
 */
template <typename T, int NumFaceTypes = 1>
class exahype2::EnclaveTask: public tarch::multicore::Task {
protected:
  typedef std::function<void()> Functor;

  const peano4::datamanagement::CellMarker _marker;
  const double                             _t;
  const double                             _dt;
  T* NOALIAS const                         _QIn;
  T* NOALIAS                               _faces[NumFaceTypes][TWO_TIMES_D];
  bool                                     _isBoundary[TWO_TIMES_D];
  T* NOALIAS const                         _QOut;
  Functor                                  _functor;

public:
  /**
   * Create plain enclave task.
   *
   * ## Create an unique id for the task type
   *
   * It is best to use the parallel factory mechanism within parallel:
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   * static int enclaveTaskTypeId = peano4::parallel::getTaskType("{{SOLVER_INSTANCE}}");
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * @param enclaveTaskTypeId Unique id for this type. If you use task
   *   fusion, only tasks of the same type will be fused.
   * @param QIn Has to be created on heap before passing to the task
   * @param QOut Has to be created on heap before passing to the task
   */
  EnclaveTask(
    int                                       enclaveTaskTypeId,
    const peano4::datamanagement::CellMarker& marker,
    double                                    t,
    double                                    dt,
    T* NOALIAS const                          QIn,
    T* NOALIAS const                          faces[NumFaceTypes][TWO_TIMES_D],
    bool                                      isBoundary[TWO_TIMES_D],
    T* NOALIAS const                          QOut,
    Functor                                   functor
  );

  EnclaveTask(const EnclaveTask&)            = delete;
  EnclaveTask& operator=(const EnclaveTask&) = delete;
  EnclaveTask(EnclaveTask&&)                 = delete;
  EnclaveTask& operator=(EnclaveTask&&)      = delete;

  virtual ~EnclaveTask() override = default;

  virtual bool run() override;
};

#include "EnclaveTask.cpph"
