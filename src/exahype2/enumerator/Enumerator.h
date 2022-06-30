// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENUMERATOR_ENUMERATOR_H_
#define _EXAHYPE2_ENUMERATOR_ENUMERATOR_H_


#include "AoSLexicographicEnumerator.h"
#include "SoALexicographicEnumerator.h"
#include "AoSoALexicographicEnumerator.h"

#include "SingleDoFEnumerator.h"


/**
 * @page Enumerator ExaHyPE 2 enumerators
 *
 *
 * ## Multipatch enumeration
 *
 * numberOfPatches_ This field is irrelevant for this enumerator,
     *   as the patches are enumerated one after another, i.e. there is no
     *   patch interleaving. To be formally correct, you should set it to 1
     *   if you construct and enumerator over the input or output data, as
     *   there is one patch per cell.
 *
 */
#endif
