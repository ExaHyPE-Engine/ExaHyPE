// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _OTTER_H_
#define _OTTER_H_


#include "../config.h"


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

  #if !defined(UseOtter)
  #endif
}


#if !defined(UseOtter)
#endif



#endif
