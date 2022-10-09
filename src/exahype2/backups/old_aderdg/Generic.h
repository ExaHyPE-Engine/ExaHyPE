// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADERDG_GENERIC_H_
#define _EXAHYPE2_ADERDG_GENERIC_H_

#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>

namespace exahype2 {
  namespace aderdg {
     
     /**
      * Render a DG nodes components as a string.
      */
     std::string plotNode(
       double Q[],
       int    unknowns
     );

    /**
     * Just runs over the patch and ensures that no entry is non or infinite.
     *
     * @param[in] UIn
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] location String that tells system from where this routine got called
     */
    void validatePatch(
      const double* __restrict__ UIn,
      int                        unknowns,
      int                        auxiliaryVariables,
      int                        order,
  	  const std::string&         location = ""
    );

    /**
     * Just runs over the space-time patch and ensures that no entry is non or infinite.
     *
     * @param[in] QIn
     * @param[in] unknowns the number of PDE unknowns that we evolve
     * @param[in] auxiliaryVariables other quantities such as material parameters that we do not evolve
     * @param[in] order the DG approximation order, which corresponds to order+1 DG nodes/Lagrange basis functions per coordinate axis
     * @param[in] location String that tells system from where this routine got called
     */
    void validateSpacetimePatch(
      const double* __restrict__ QIn,
      int                        unknowns,
      int                        auxiliaryVariables,
      int                        order,
      const                      std::string& location = ""
    );
  }
}

#endif
