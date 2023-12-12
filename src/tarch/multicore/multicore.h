// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include <string>

#include "config.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(SharedOMP) || defined(SharedTBB) || defined(SharedCPP) || defined(SharedSYCL)
#define SharedMemoryParallelisation
#endif

#pragma once

namespace tarch {

  /**

 @namespace tarch::multicore


 This page describes Peano 4's multithreading layer.
 To compile with multicore support, you have to invoke the configure script with
 the option --with-multithreading=value where value is

 - cpp. This adds support through C++14 threads.
 - tbb. This adds support through Intel's Threading Building Blocks. If you use
   this option, you first have to ensure that your CXXFLAGS and LDFLAGS point to
   the right include or library, respectively, directories. LDFLAGS also has to
   compromise either -ltbb or -tbb_debug.
 - openmp. This adds OpenMP support. We currently develop against OpenMP 4.x
   though some of our routines use OpenMP target and thus are developed against
   OpenMP 5.
 - sycl. We have a SYCL support for the multithreading, through, within the Intel
   toolchain, it might be more appropriate to combine sycl on the GPU with the
   tbb backend for multithreading.


 ## Writing your own code with multithreading features

 If you wanna distinguish in your code between multicore and no-multicore variants,
 please use

~~~~~~~~~~~~~~~~~~~~~~~~
#include "tarch/multicore/multicore.h"
~~~~~~~~~~~~~~~~~~~~~~~~

and

~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(SharedMemoryParallelisation)
~~~~~~~~~~~~~~~~~~~~~~~~


 With the symbol SharedMemoryParallelisation, you make your code independent of
 OpenMP, TBB or C++ threading.


 Our vision is that each code should be totally independent of the multithreading
 implementation chosen. Indeed, Peano 4 itself does not contain any direct
 multithreading library calls. It solely relies on the classes and functions from
 tarch::multicore.


 ## Multicore architecture

 The multithreading environment is realised through a small set of classes. User
 codes work with these classes. Each type/function has an implementation within
 src/multicore. This implementation is a dummy that ensures that all code works
 properly without any multithreading support. Subdirectories hold alternative
 implementations (backends) which are enabled once the user selects a certain multithreading
 implementation variant, i.e. depending on the ifdefs set, one of the
 subdirectories is used. Some implementations introduce further headers, but user
 code is never supposed to work against functions or classes held within
 subdirectories.

  @image html multicore_architecture.png

 The central instance managing the threads on a system is tarch::multicore::Core.
 This is a singleton and the name thus is slightly wrong. It does not really represent
 one core but rather represents the landscape of cores. You can setup the
 multithreading environment through Core's configure() routine, but this is optional.
 Indeed, multithreading should work without calling configure() at all. Each
 multitheading backend offers its own realisation of the
 Core class.

 For multithreaded code, it is important that the code can lock (protect) code
 regions and free them. For this, the multithreading layer offers different
 semaphores. Each multithreading backend maps these logical concepts onto its
 internal synchronisation mechanism. Usually, I use the semaphores through lock
 objects. As they rely on the semaphore implementations, they are generic and work
 for any backend.


 ## Task model

 Peano models all of its interna as tasks. Each Peano 4 task is a subclass of
 tarch::multicore::Task. However, these classes might not be mapped 1:1 onto
 native tasks. Indeed we distinguish different task types:

 - Tasks. The most generic type of tasks is submitted via spawnTask(). Each
   task can be assigned a unique number and incoming dependencies.
 - Fork-join tasks. These are created via tarch::multicore::spawnAndWait()
   and form a subset of the generic tasks. Here, we know the dependency
   structure and waits quite explicitly, so there's no need to work with task
   numbers.
 - Free floating tasks (task sinks). Tasks without any outgoing dependencies
   are free floating in the sense that we never wait for them. That's obviously
   almost never true - few tasks have no follow-up dependencies at all - but
   the user might decide to model the out dependencies without task
   dependencies: Typically, such tasks set some output flag or dump their
   result into a database, which then in turn unlocks follow-up tasks.
 - Pending tasks: Pending tasks are ready tasks which Peano's tasking API
   holds back on purpose. They are not (yet) submitted to the tasking
   system, as we might want to fuse them into larger task assemblies (and
   move them to other ranks/devices, e.g.).


 ## Vanilla Peano task graph

 The two routines allow us to model the typical Peano 4 task graph:

 @image html multicore_task_flow.png

 Peano's main core realises a classic fork-join parallelism (left) where the
 fork-join segments are realised via tasks. The routine tarch::multicore::spawnAndWait()
 is used to construct this task graph part on-the-fly. Formally, this part of the
 code is very simple to classic BSP (cmp OpenMP's parallel for), which is recursively
 nested into each other.

 Each task within the core fork-join DAG might spawn additional tasks.
 When we wait for the BSP part of the graph (left) to terminate, these tasks
 still might linger around. They should backfill any empty task queue when it
 is appropriate or, in general, be executed with low priority. Late on
 throughout the execution, some further tasks for the core code section
 will need the outcome of tasks that have been spawned before. At this
 point, the tasks should already be completed (and therefore have dumped
 their outcomes into a database). Otherwise, we have to manually invoke
 the scheduler manually via tarch::multicore::processPendingTasks().


 The tasks to the right are typically called enclave tasks, and they are
 typically added on top of the core fork-join task structure by Peano
 extensions. The prime example is ExaHyPE 2. Enclave tasking yields real bursts
 of low priority tasks, and it would be unacceptable complicated to model the
 dependencies on their outcomes via a task graph. Therefore, I went down the
 route that these tasks dump their outcomes into a table, and consumer tasks
 (or receivers in C++ terminology) then take the outcomes from there.

 There is a further reason to work with the dedicated queues: As we found out
 and describe in the 2021 IWOMP paper by H. Schulz et al, tasking frameworks
 such as OpenMP might decide to switch to any ready task throughout the
 execution. This means that they might postpone the processing of the
 DAG to the left and instead do the free tasks on the right. This is not
 what we want: We want them to have lower priority. An important paper to read
 in the context of the present solution is also
 https://www.osti.gov/pages/servlets/purl/1465188. Note in particular the
 example in Figure 3.


 ### GPUs and an additional queue (pending tasks)

 On some platforms and code generations, it has proven of value to add an
 additional runtime (layer) on top of the native tasking backend. These layers
 do exist for TBB and OpenMP, e.g., and all follow the same general pattern.
 Basically, they introduce an additional, user-managed task queue on top of the
 native tasking systems. Enclave tasks are not spawned into the native runtime
 but instead held back within this additional queue. There are two advantages
 of this:

 1. Some runtimes see the task spawning as scheduling point (OpenMP e.g.) and
    might decide to run the enclave tasks immediately, as there's so many of
    them. However, we really want to have them low priority. By holding them
    back in our own queue, they are invisible to the tasking runtime and
    therefore cannot be scheduled.

 2. We can search the additional queue of tasks of the same type which can fit
    to the GPU. If we find multiple of these guys, we pack them together into
    one large meta task (or task assembly) and throw them onto an accelerator.

 Performance analysis shows that using one global queue on top of the actual
 tasking backend quickly introduces performance issues, as too many tasks
 might try to spawn their tasks concurrently into this queue. We suffer from
 congestion. Therefore, the implementation in Tasks.cpp employs one global
 queue plus a queue per thread.


 ### Tasks with dependencies

 In Peano, task DAGs are built up along the task workflow. That is, each task
 that is not used within a fork-join region or is totally free is assigned a
 unique number when we spawn it.

 Whenever we define a task, we can also define its dependencies. This is a sole
 completion dependency: you tell the task system which task has to be completed
 before the currently submitted one is allwed to start. A DAG thus can be built
 up layer by layer. We start with the first task. This task might be
 immediately executed - we do not care - and then we continue to work our way
 down through the graph adding node by node.

 Different to OpenMP, outgoing dependencies do not have to be declared. We
 solely model in dependencies. This however imples that all predecessors of a
 task have to be submitted before we add this very task. This is not always
 possible. When you walk top-down through a tree and then bottom-up again,
 you might have thrown away finer levels when you get back to the original
 one and thus you are unable to introduce dependencies from fine to coarse
 unless you introduce very fancy bookkeeping.

 We avoid such bookkeeping (or deploy it into the tasking API), as we allow
 codes to register a task. registerTask() tells the system "there will be a
 task eventually, but I haven't yet constructed it". You can also add
 dependencies from an existing task to such a registered task. Once you
 finally submit the registered task, you are however not allowed to add any
 dependencies anymore.



 ### Orchestration and (auto-)tuning

 The actual orchestration is controlled via an implementation of
 tarch::multicore::orchestration::Strategy that you set via
 tarch::multicore::setOrchestration(). The strategy basically controls:

 - How many enclave tasks should be hold back in the user-defined queue. If
   you exceed this treshold, the backend maps each enclave task 1:1 onto a
   native task.
 - How many tasks should the system try to fuse into one GPU call.
 - Should the code try to fuse these tasks immediately when they are spawned
   or search for fusion candidates every time a fork-join section has
   terminated.
 - What is an appropriate scheduling strategy for a fork-join section depending
   on the level of nested parallelisation calls.

 The class tarch::multicore::orchestration::StrategyFactory allows you to pick
 various common strategies, and it also provides the routine which determines
 the default variant which is chosen if the user does not manually pick one.
 As we outsource the orchestration into strategy objects, users can implement
 online and offline autotuning within through an implementation of
 tarch::multicore::orchestration::Strategy.


 ### Scheduling flavours for fork-join sections

 Whenever we hit a fork-join section, i.e. encounter tarch::multicore::spawnAndWait(),
 there are different scheduling variants on the table:

 1. Execute the tasks straightaway. Do not exploit any concurrency.
 2. Run a straightforward, native task loop.
 3. Run through the forked tasks in parallel and check eventually if we should
    map some subtasks onto native tasks.

 The orchestration of choice can control this behaviour via
 tarch::multicore::orchestration::Strategy::paralleliseForkJoinSection().

 The documentation of tarch::multicore::spawnAndWait()
 provides rationale why I think that variant (2) minimises the algorithmic
 latency, whereas variant (3) maximises occupation, and


 ## Backends

 ### OpenMP

 If you want to use the OpenMP backend, you have to embed your whole main loop
 within an

~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma omp parallel
#pragma omp single
{
~~~~~~~~~~~~~~~~~~~~~~~~~

 environment. Furthermore, you will have to use

~~~~~~~~~~~~~~~~~~~~~~~~~
  export OMP_NESTED=true
~~~~~~~~~~~~~~~~~~~~~~~~~

 on some systems, as we rely heavily on nested parallelism.


 ## Statistics

 If the Peano statistics are enabled, the tasking backend will sample some quantities:

 - "tarch::multicore::bsp-concurrency-level" Typically corresponds to the
   number of fork-join traversal tasks.
 - "tarch::multicore::global-pending-tasks" Global pending tasks.
 - "tarch::multicore::thread-local-pending-tasks" Pending tasks per thread
   which are not yet committed to the global queue.
 - "tarch::multicore::fuse-tasks" Number of tasks which have been fused.

 Depending on the chosen backend, you might get additional counters on top.

 */
  namespace multicore {
    /**
     * Switch on SmartMPI
     *
     * If you use SmartMPI, then the bookkeeping registers the the local scheduling.
     * If you don't use SmartMPI, this operation becomes nop, i.e. you can always
     * call it and configure will decide whether it does something useful.
     */
    void initSmartMPI();
    void shutdownSmartMPI();
  } // namespace multicore
} // namespace tarch
