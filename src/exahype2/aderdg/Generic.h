// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADERDG_GENERIC_H_
#define _EXAHYPE2_ADERDG_GENERIC_H_



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>

// @todo: only 2D case considered atm
namespace exahype2 {
  namespace aderdg {
    /**
     * Just runs over the patch and ensures that no entry is non or infinite.
     *
     * @param location String that tells system from where this routine got called
     */
    void validatePatch(
      double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    order,
  	  const std::string& location = ""
    );

    void validateSpacetimePatch(
      double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    order,
      const std::string& location = ""
    );
  }
}

#endif
