// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "config.h"


/**
 * @page tarch_multicore_otter Otter task tracing
 *
 *
 * Otter is a thread tracing and visualisation tool, and it also can predict the
 * impact of task parallelism on the performance and hence provide valuable
 * insight.
 * It is able to guide the development of core components of Peano.
 * For applications built on top of Peano such as ExaHyPE or Swift, it provides
 * information how to schedule tasks efficiently, i.e. how to map various tasks
 * onto various scheduling policies.
 * Otter is hosted at https://github.com/Otter-Taskification/otter/wiki.
 *
 *
 * ## Installation with Otter support
 *
 * To build Peano against Otter, you have to add --with-otter on the
 * configuration prompt or to add the respective extension to your cmake call.
 * It is not enabled by default. Otter itself is based upon CMake. You might
 * want to install it on your system, but a simple make usually does the
 * job as long as you get a build directly within Otter which hosts all the
 * libraries and includes.
 *
 * - Add -Imyotter/include/api to your CFLAGS.
 * - Add -Imyotter/build/include to your CFLAGS.
 *
 *
 * ## Using Otter
 *
 * The single point of contact for otter is the file otter.h. This
 * file has two purposes:
 *
 * - It defines some Peano-specific Otter constants, such as global Otter
 *   identifiers that I then use consistently throughout the project.
 * - It defines all Otter macros and functions as empty instructions. This part
 *   is commented out if you use Otter actually. If you don't use it, then my
 *   dummies from this file are active.
 *
 * The nice thing about the latter approach is that Peano works without ifdefs.
 * The drawback is that we have to ensure that the functions and macros here
 * are always consistent with the latest Otter version. As a user, you can use
 *
 *           #ifdef UseOtter
 *
 * from the config.h, but I do recommend to simply invoke the otter macros from
 * otter.h. If otter is not included in the build, they will degenerate to nop.
 *
 * Peano uses Otter's task graph API. There are a few other flavours of the
 * API, but that's the one we use. Fortunately, this API provides an empty
 * version of all macros already. Therefore, it is sufficient to copy over the
 * macros from otter-task-graph-stub into this header whenever we run into
 * incompatibilities, i.e. whenever Otter has updated its signatures.
 */
namespace otter {

  #if !defined(UseOtter)
  #endif
}


#if defined(UseOtter)
#include "otter-task-graph/otter-task-graph.h"
#else

// use sub header:
// https://github.com/Otter-Taskification/otter/blob/dev/include/otter/otter-task-graph-stub.h

#define OTTER_SRC_ARGS()
#define otter_task_context void
#define otter_sync_children
#define otter_sync_descendants
#define otterTraceInitialise()
#define otterTraceFinalise()
#define otterTraceStart()
#define otterTraceStop()
#define otterTaskBegin(...) ((void*)0)
#define otterTaskEnd(...)
#define otterSynchroniseTasks(...)
#define otterPhaseBegin(...)
#define otterPhaseEnd()
#define otterPhaseSwitch(...)

#endif
