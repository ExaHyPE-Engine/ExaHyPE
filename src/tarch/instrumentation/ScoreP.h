// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * Score-P user-instrumentation wrapper. Without Score-P, SCOREP_USER_REGION reads its name and every
 * other region macro expands to nothing. A name that only a region uses then counts as referenced.
 */
#if defined(SCOREP_USER_ENABLE)
#include <scorep/SCOREP_User.h>
#else
#define SCOREP_USER_REGION(name, type) static_cast<void>(name);
#define SCOREP_USER_REGION_DEFINE(handle)
#define SCOREP_USER_REGION_BEGIN(handle, name, type)
#define SCOREP_USER_REGION_END(handle)
#define SCOREP_USER_REGION_TYPE_COMMON 0
#define SCOREP_USER_REGION_TYPE_FUNCTION 0
#define SCOREP_USER_REGION_TYPE_LOOP 0
#define SCOREP_USER_REGION_TYPE_DYNAMIC 0
#define SCOREP_USER_REGION_TYPE_PHASE 0
#endif
