// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org


#include "config.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#include <string>


#if defined(SharedOMP) || defined(SharedTBB) || defined(SharedCPP) || defined(SharedSYCL)
  #define SharedMemoryParallelisation
#endif


#ifndef _TARCH_MULTICORE_MULTICORE_H_
#define _TARCH_MULTICORE_MULTICORE_H_


/**
 * Have to include this header, as I need access to the SYCL_EXTERNAL keyword.
 */
#include "tarch/multicore/sycl/SYCL.h"


namespace tarch {


  /**
 
 \namespace "tarch::multicore"

 <h1> Peano 4's multithreading layer </h1>

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


 <h2> Using multithreading (user view) </h2>

 See the guidebook and the configure script's help please. As a summary, it should
 be sufficient to translate with
 \code
 ./configure ... --with-multithreading=xxx
 \endcode

 You might furthermore want to use particular vendor toolchains or enable GPU
 support.


 <h2> Writing your own code with multithreading features </h2>

 If you wanna distinguish in your code between multicore and no-multicore variants, 
 please use
\code
#include "tarch/multicore/multicore.h"
\endcode
and  
\code
#if defined(SharedMemoryParallelisation)
\endcode
 With the symbol SharedMemoryParallelisation, you make your code independent of
 OpenMP, TBB or C++ threading.

 Our vision is that each code should be totally independent of the multithreading
 implementation chosen. Indeed, Peano 4 itself does not contain any direct
 multithreading library calls. It solely relies on the classes and functions from
 this namespace.


 <h2> Multicore architecture </h2>

 The multithreading environment is realised through a small set of classes. User
 codes work with these classes. Each type/function has an implementation within
 src/multicore. This implementation is a dummy that ensures that all code works
 properly without any multithreading support. Subdirectories hold alternative
 implementations (backends) which are enabled once the user selects a certain multithreading
 implementation variant, i.e. depending on the ifdefs set, one of the
 subdirectories is used. Some implementations introduce further headers, but user
 code is never supposed to work against functions or classes held within
 subdirectories.

 The central instance managing the threads on a system is tarch::multicore::Core.
 This is a singleton and the name thus is slighly wrong. It does not really represent
 one core but rather represents the landscape of cores. You can setup the
 multithreading environment through Core's configure() routine, but this is optional.
 Indeed, multithreading should work without calling configure() at all. Each
 multitheading backend (OpenMP, C++, TBB) offers its own realisation of the
 Core class.

 For multithreaded code, it is important that the code can lock (protect) code
 regions and free them. For this, the multithreading layer offers different
 semaphores. Each multithreading backend maps these logical concepts onto its
 internal synchronisation mechanism. Usually, I use the semaphores through lock
 objects. As they rely on the semaphore implementations, they are generic and work
 for any backend.


 <h3> Logical task model </h3>

 Peano models all of its interna as tasks. Each Peano 4 task is a subclass of
 tarch::multicore::Task. Peano's task are an abstraction from OpenMP or TBB which
 is rather puristic: Besides a run() routine which can ask the task system to rerun
 the task - this is a built-in yield() mechanism - they have priorities and a
 prefetch() operation, i.e. a wake-up routine that should be called before a task
 is actually executed yet is optional. Obviously, most task backends do offer all
 of these features directly, i.e. we can map 1:1, but in Peano 4 this is hidden from
 the user. Our idea is that we can swap backends in and out.


 The user or Peano 4 core,
 respectively, create Peano 4 tasks and spawn them through tarch::multicore::spawnTask(),
 e.g. It is up to the backend to map these spawn routines and the task classes
 onto a particular implementation/runtime.

 Peano 4 has two types of spawn mechanisms: spawn-and-wait and spawn (and forget).
 Forget tasks typically aren't real "do them at one point and then forget them"
 tasks, so this nomenclature is not watertight. Typically, they do meaningful
 stuff and then set some semaphore or similar. The spawn-and-forget means that they
 are not immediately due, i.e. do not induce algorithmic latency. Therefore, I
 should better call them background tasks or similar.

 The main code spawns these background tasks, and it then eventually waits for them to complete. While
 it waits, it will/should call tarch::multicore::processPendingTasks() until the
 particular task it waits for has completed. I do not offer a way to process a
 particular task upon demand. So the overall vision is: background task should go
 into some kind of workpool and backfill idle cores. In the ideal case, they are
 done by the time we need their outcome (which is at a later point - more critical
 tasks are scheduled via a classic tree-dependency thing), but if they are not I
 will call processPendingTasks() and thus give the runtime the opportunity to
 catch up.

 <h3> Peano's task flow </h3>

 Peano 4 can be read as one big BSP code which spawns additional tasks. That is, the
 code runs through the mesh with one big parallel for, waits for the for to terminate,
 and then continues.

 The caveat is that each parallel for might spawn additional tasks.
 When we wait for the BSP to terminate, these tasks still might linger around (see
 discussion on spawn mechanisms above).
 They should backfill the queues when it is appropriate.
 In one of the subsequent parallel fors, the code will need the outcome of these tasks
 that have been spawned before.
 It waits for them and, while it waits, calls processPendingTasks().


 <h3> Native backends </h3>

 I provide a couple of native backends where the Peano's task architecture is mapped
 1:1 onto the backend. At the moment, these are

 - TBB
 - OpenMP

 You find the implementations in the respective directories. The files are called
 Tasks_plain.cpp. They are very simple and thus should be digestable without further
 comments. For C++ threads, I do not offer a plain implementation at the moment, but
 do offer solely a variant with its own tasking runtime layer (see below).


 <h3> Backends with additional runtime layer </h3>

  @image html multicore_architecture.png

 On some platforms and code generations, it has proven of value to add an additional
 runtime (layer) on top of the backend. These layers do exist for TBB and OpenMP, e.g.,
 and all follow the same general patterns. The patterns in turn are inspired
 by Intel's TBB, some of them result from papers we've written, for which we had
 to pimp the scheduling. The discussion how to realise the tasking mainly focuses
 on what happens if the user spawns a task and immediately returns. All tasking
 schemes provide features where a user passes in a set of tasks and returns
 immediately. This is classic bulk synchronous processsing (BSP). The other stuff
 is the challenging part.

 \image html multicore_task_flow.png

 Our Peano 4 task are a logical concept. They have nothing to do with threads or
 tasks in an OpenMP or TBB sense. Indeed, the individual tasking backends offer
 different mappings of the logical tasks onto parallelism:

 - We can either not use any tasking at all;
 - or map all logical tasks directly onto real tasks;
 - or store logical tasks in a local queue and decide case by case whether we
   map them onto proper tasks or do something else with them, i.e. process
   them immediately.

 Consult the orchestration documentation for details.


  <h2> OpenMP backend </h2>

 If you want to use the OpenMP backend, you have to embed your whole main loop
 within an

 <pre>
#pragma omp parallel
#pragma omp single
{
 </pre>

 environment. Furthermore, you will have to use

 <pre>
  export OMP_NESTED=true
 </pre>

 on some systems, as we rely heavily on nested parallelism.

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
  }

  enum class MemoryLocation {
    /**
     * Create data on the heap of the local device. That is, if you run
     * on an accelerator, use the accelerator's heap. If you invoke it
     * on the host, use the host's accelerator.
     */
    Heap,
    /**
     * To be used on host only.
     */
    ManagedAcceleratorMemory
  };



// @todo Should not be required on the long run, as we want to map this via GPUCallableMethod
//       Remove if not required
#if defined(_NVHPC_STDPAR_GPU)
 #define HOST_DEVICE __host__ __device__
 #define HOST __host__
 #define DEVICE __device__
#else
 #define HOST_DEVICE
 #define HOST
 #define DEVICE
#endif



  /**
   * We need an annotation GPUCallableMethod for some targets like HIP. This
   * marker is to be used before the return type is written down. This is different
   * to the InlineMethod marker which is to be used after the argument list.
   */
  #if defined(GPUOffloadingHIP)
    #define GPUCallableMethod __host__ __device__
  #elif defined(GPUOffloadingSYCL)
    #define GPUCallableMethod SYCL_EXTERNAL
  #elif defined(_NVHPC_STDPAR_GPU) && defined(GPUOffloadingCPP)
    #define GPUCallableMethod __host__ __device__
  #else
    #define GPUCallableMethod
  #endif


  /**
   * This is the marker that is to be used after the argument list of a
   * function declaration.
   */
  #if defined(UseManualInlining)
    #define InlineMethod __attribute__((always_inline))
  #else
    #define InlineMethod
  #endif


  double* allocateMemory(int size, MemoryLocation location);
  void freeMemory(double* data, MemoryLocation location);


  /**
   * Delegates to std::abort() if no GPU offloading is active. Otherwise is degenerates
   * to nop, as there is no abort on an accelerator.
   */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  void gpuAbort();
  #pragma omp end declare target
  #elif defined(GPUOffloadingSYCL)
  GPUCallableMethod void gpuAbort();
  #elif defined(GPUOffloadingCPP)
  HOST_DEVICE void gpuAbort();
  #else
  void gpuAbort();
  #endif
}

#endif
