// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FaceIntegral.h"

#include "DGMatrices.h"
#include "Quadrature.h"

#include "tarch/CompilerSpecificSettings.h"

template <typename SolutionType, typename cCompType>
void {{FULL_QUALIFIED_NAMESPACE}}::faceIntegral(
  SolutionType*           lduh,
  const cCompType* const  lFhbnd,
  const int               direction,
  const int               orientation,
  const double            inverseDxDirection,
  const double            timeStepSize
) {
  const cCompType* const FCoeff = (orientation == 0 ? DGMatrices<cCompType>::FLCoeff : DGMatrices<cCompType>::FRCoeff);

{% if IS_LINEAR %}
  const cCompType scaling = inverseDxDirection;
{% else %}
  const cCompType scaling = (2.0 * orientation - 1.0) * inverseDxDirection;
{% endif %}

  switch (direction) {
  case 0: // x-faces, left-right flux.
    for (int zy = 0; zy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; zy++) {
      const cCompType scaledWeight = scaling * timeStepSize * Quadrature<cCompType>::weights2[zy];
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        SIMD(aligned(lduh,FCoeff,lFhbnd:ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
          lduh[(zy * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA}} + n] -= Quadrature<cCompType>::iweights3[zy * {{NUMBER_OF_DOF}} + x]
            * scaledWeight * FCoeff[x] * lFhbnd[zy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        }
      }
    }
    break;
  case 1: // y-faces
    for (int z = 0; z < {{NUMBER_OF_DOF_3D}}; z++) {
      for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
        const cCompType scaledWeight = scaling * timeStepSize * Quadrature<cCompType>::weights2[z * {{NUMBER_OF_DOF}} + x];
        for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
          SIMD(aligned(lduh,FCoeff,lFhbnd:ALIGNMENT))
          for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
            lduh[((z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_DATA}} + n] -= Quadrature<cCompType>::iweights3[(z * {{NUMBER_OF_DOF}} + y) * {{NUMBER_OF_DOF}} + x]
              * scaledWeight * FCoeff[y] * lFhbnd[(z * {{NUMBER_OF_DOF}} + x) * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
          }
        }
      }
    }
    break;
  case 2: // z-faces
    for (int yx = 0; yx < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; yx++) {
      const cCompType scaledWeight = scaling * timeStepSize * Quadrature<cCompType>::weights2[yx];
      for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
        SIMD(aligned(lduh,FCoeff,lFhbnd:ALIGNMENT))
        for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
          lduh[(z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + yx) * {{NUMBER_OF_DATA}} + n] -= Quadrature<cCompType>::iweights3[z * {{NUMBER_OF_DOF*NUMBER_OF_DOF}} + yx]
            * scaledWeight * FCoeff[z] * lFhbnd[yx * {{NUMBER_OF_UNKNOWNS_PADDED}} + n];
        }
      }
    }
    break;
  }
}

template void {{FULL_QUALIFIED_NAMESPACE}}::faceIntegral(
  {{SOLUTION_STORAGE_PRECISION}}*                   lduh,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const  lFhbnd,
  const int                                         direction,
  const int                                         orientation,
  const double                                      inverseDxDirection,
  const double                                      timeStepSize
);
