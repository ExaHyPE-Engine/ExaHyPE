/**
 * This file is part of the ExaHyPE project.
 * Copyright (c) 2016  http://exahype.eu
 * All rights reserved.
 *
 * The project has received funding from the European Union's Horizon 
 * 2020 research and innovation programme under grant agreement
 * No 671698. For copyrights and licensing, please consult the webpage.
 *
 * Released under the BSD 3 Open Source License.
 * For the full license text, see LICENSE.txt
 **/
 
#include "GaussLegendreBasis.h"
#include "peano4/utils/Loop.h"

// Snippet generated with ExaHyPE-Engine/Miscellaneous/aderdg/generateLookupTable.py

#include "generated/GaussLegendreBasis.csnippet"

double kernels::legendre::interpolate(
  [[maybe_unused]] const double* offsetOfPatch,
  [[maybe_unused]] const double* sizeOfPatch,
  [[maybe_unused]] const double* x,
  [[maybe_unused]] int           numberOfUnknowns,
  [[maybe_unused]] int           unknown,
  [[maybe_unused]] int           order,
  [[maybe_unused]] const double* u
) {
  double result = 0.0;

  [[maybe_unused]] double xRef[Dimensions];
  xRef[0] =  (x[0] - offsetOfPatch[0]) / sizeOfPatch[0];
  xRef[1] =  (x[1] - offsetOfPatch[1]) / sizeOfPatch[1];
  #if Dimensions==3
  xRef[2] =  (x[2] - offsetOfPatch[2]) / sizeOfPatch[2];
  #endif

  // The code below evaluates the basis functions at the reference coordinates
  // and multiplies them with their respective coefficient.
  /*
  dfor(ii,order+1) { // Gauss-Legendre node indices
    int iGauss = peano4::utils::dLinearisedWithoutLookup(ii,order + 1);
    result += kernels::legendre::basisFunction[order][ii(0)](xRef[0]) *
              kernels::legendre::basisFunction[order][ii(1)](xRef[1]) *
              #if Dimensions==3
              kernels::legendre::basisFunction[order][ii(2)](xRef[2]) *
              #endif
               u[iGauss * numberOfUnknowns + unknown];
    assertion6(std::isfinite(result), result, unknown, iGauss, numberOfUnknowns, offsetOfPatch[0], sizeOfPatch[0]);
  }
  */

  return result;
}
