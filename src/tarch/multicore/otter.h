// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "config.h"
#define OTTER_ROOT_PHASES_MODE 2

#if OTTER_ROOT_PHASES_MODE == 1
#define OTTER_ROOT_TASK_EXPLICIT 1
#define OTTER_USE_PHASES 0
#elif OTTER_ROOT_PHASES_MODE == 2
#define OTTER_ROOT_TASK_EXPLICIT 0
#define OTTER_USE_PHASES 1
#endif

/**
 * @namespace otter
 *
 * Peano can link to Otter, the ExCALIBUR X-cutting theme's core tracing tool. This
 * file has two purposes:
 *
 * - It defines some Peano-specific Otter constants, such as global Otter identifiers
 *   that I ten use consistently throughout the project.
 * - It defines all Otter macros and functions as empty instructions. This part is
 *   commented out if you use Otter actually. If you don't use it, then my dummies from
 *   this file are active. This way, I don't need ifdefs within the code. I can always
 *   work against Otter macros and functions.
 *
 * The nice thing about the latter approach is that Peano works without ifdefs and can
 * consequently use Otter. The drawback is that we have to ensure that the functions
 * and macros here are always consistent with the latest Otter version.
 */
namespace otter {
namespace label{
    static const char *root = "[ROOT TASK]";
    static const char *step = "[STEP TASK]";
    static const char *enclave = "[anonymous enclave task]";
    static const char *traverse_fmt = "[traversal task %d of %d]";
    static const char *unknown = "unknown task";
    static const char *TaskWithCopyOfFunctor = "TaskWithCopyOfFunctor";
    static const char *TaskWithoutCopyOfFunctor = "TaskWithoutCopyOfFunctor";
}

namespace phase {
    static const char *init = "init";
    static const char *plot = "plot";
    static const char *timestep = "time-step";
}

}

#if defined(UseOtter)
#include <otter/otter-task-graph-user.h>
#else

#define OTTER_INITIALISE()
#define OTTER_FINALISE()
#define OTTER_DECLARE_HANDLE(...)
#define OTTER_INIT_TASK(...)
#define OTTER_ADD_TO_POOL(...)
#define OTTER_REMOVE_FROM_POOL(...)
#define OTTER_BORROW_FROM_POOL(...)
#define OTTER_TASK_START(...)
#define OTTER_TASK_END(...)
#define OTTER_TASK_WAIT_FOR(...)
#define OTTER_PHASE_BEGIN(...)
#define OTTER_PHASE_END(...)
#define OTTER_PHASE_SWITCH(...)

#endif
