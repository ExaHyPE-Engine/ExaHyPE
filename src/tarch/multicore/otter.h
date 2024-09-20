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
 * It is not enabled by default. Otter itself is based upon CMake. See the
 * Otter documentation linked above for instructions on building and installing
 * it. In the remaining text I assume that you have typed in
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 * mkdir build
 * cd build
 *
 * cmake ..
 * cmake --build .
 * cmake --install . --prefix otter-dir
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * so Otter is properly installed. There are only few steps required to build
 * the system:
 *
 * - Add --with-otter to your configure call.
 * - Augment your CXXFLAGS="... -Iotter-dir/include".
 * - Augment your LDFLAGS="... -Lotter-dir/libs".
 * - Augment your LIBS="... -lotter-task-graph".
 *
 *
 *
 * OTF2 is the only mandatory dependency of Otter. If you have built it
 * as static library, the Otter libraries contains it already, and nothing
 * is to be done: We don't need to add these libraries to Peano's
 * configuration. If you have built it as dynamic library - which is the
 * default on most Ubuntu systems, e.g. - you have to add it to the linker
 * call explicitly:
 *
 * - Augment your LDFLAGS="... -L/opt/otf2/lib" or wherever OTF did and up.
 * - Augment your LIBS="... -lotf2".
 *
 *
 *
 * A similar argument holds for HDF5. If you use HDF5 within Otter - which is
 * optional - you will have to add its shared libraries
 *
 *             LIBS="... -lhdf5_hl_cpp -lhdf5_cpp -lhdf5_hl -lhdf5"
 *
 * to Peano's configuration, too.
 *
 *
 * It is important that the Otter libs are enlisted after all other libraries
 * of Peano, and that the HDF5 and OTF libs are in turn
 * enlisted ***after*** the otter libs, as C++'s linker reads libraries from
 * left to right and resolves dependencies from left to right.
 *
 *
 * Analogous steps are required if you use cmake.
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
  namespace label {
    static const char* step                     = "[STEP TASK]";
  } // namespace label

  namespace phase {
    static const char* init                  = "init";
    static const char* plot                  = "plot";
    static const char* timestep              = "time-step";
    static const char* create_grid           = "create-grid";
    static const char* create_grid_no_refine = "create-grid-but-postpone-refinement";
    static const char* create_grid_converge  = "create-grid-and-converge-load-balancing";
  } // namespace phase

} // namespace otter

#if defined(UseOtter)
#include <otter/otter-task-graph-user.h>
#else

#define OTTER_UTIL_ASSERT(...)
#define OTTER_INITIALISE()
#define OTTER_FINALISE()
#define OTTER_DECLARE_HANDLE(...)
#define OTTER_INIT_TASK(...)
#define OTTER_DEFINE_TASK(...)
#define OTTER_POOL_ADD(...)
#define OTTER_POOL_POP(...)
#define OTTER_POOL_DECL_POP(...)
#define OTTER_POOL_BORROW(...)
#define OTTER_POOL_DECL_BORROW(...)
#define OTTER_POOL_SIZE(...)
#define OTTER_TASK_START(...)
#define OTTER_TASK_END(...)
#define OTTER_TASK_WAIT_START(...)
#define OTTER_TASK_WAIT_END(...)
#define OTTER_TASK_YIELD_START(...)
#define OTTER_TASK_YIELD_END(...)
#define OTTER_TASK_WAIT_IMPLICIT(...)
#define OTTER_PHASE_BEGIN(...)
#define OTTER_PHASE_END(...)
#define OTTER_PHASE_SWITCH(...)

#endif
