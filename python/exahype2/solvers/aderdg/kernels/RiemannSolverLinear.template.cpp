// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "DGMatrices.h"
#include "Quadrature.h"

#include <algorithm>
#include <cstring>
#include <cmath>

template <typename cCompType>
void {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  cCompType* __restrict__                       FL,
  cCompType* __restrict__                       FR,
  const cCompType* __restrict__ const           QL,
  const cCompType* __restrict__ const           QR,
  const double                                  timeStamp,
  const double                                  timeStepSize,
  const tarch::la::Vector<Dimensions, double>&  faceCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const int                                     direction
) {
  /*
   * For the linear kernels, we need the material parameters in the
   * extrapolated predictor.
   * We compute the averages of the material parameters but
   * do not use them in the max eigenvalue calculation.
   */
  std::memset(FL, 0, {{NUMBER_OF_UNKNOWNS_PADDED * NUMBER_OF_DOF * NUMBER_OF_DOF_3D}} * sizeof(cCompType));
  std::memset(FR, 0, {{NUMBER_OF_UNKNOWNS_PADDED * NUMBER_OF_DOF * NUMBER_OF_DOF_3D}} * sizeof(cCompType));

  // Compute the average unkown and auxiliary variables from the left and the right
  cCompType QavL[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
  cCompType QavR[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};

  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    #ifdef SharedOMP
    #pragma omp simd aligned(QavL,QavR,QL,QR:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_DATA_PADDED}}; n++) {
      QavL[n] += Quadrature<cCompType>::weights2[xy] * QL[xy * {{NUMBER_OF_DATA_PADDED}} + n];
      QavR[n] += Quadrature<cCompType>::weights2[xy] * QR[xy * {{NUMBER_OF_DATA_PADDED}} + n];
    }
  }

  cCompType smaxL = solver.maxEigenvalue(
    &QavL[0],
    faceCentre,
    cellSize,
    timeStamp,
    timeStepSize,
    direction
  );
  cCompType smaxR = solver.maxEigenvalue(
    &QavR[0],
    faceCentre,
    cellSize,
    timeStamp,
    timeStepSize,
    direction
  );
  cCompType smax = std::max(smaxL, smaxR);

  cCompType Qavg[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));

  #ifdef SharedOMP
  #pragma omp simd aligned(Qavg,QavR,QavL:{{ALIGNMENT}})
  #endif
  for (int n = 0; n < {{NUMBER_OF_DATA_PADDED}}; n++) {
    Qavg[n] = 0.5 *  (QavR[n] + QavL[n]);
  }

  cCompType Qdiff[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned({{ALIGNMENT}})));

{% if NUMBER_OF_AUXILIARY_VARIABLES > 0 %}
  // We copy the averaged material parameters to Qdiff as they are used in the flux term
  std::copy_n(Qavg + {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_DATA_PADDED - NUMBER_OF_UNKNOWNS}}, Qdiff + {{NUMBER_OF_UNKNOWNS}});
{% endif %}

{% if USE_NCP %}
  cCompType ncp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
{% endif %}

{% if USE_FLUX %}
  cCompType fluxValues[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
{% endif %}

  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    #ifdef SharedOMP
    #pragma omp simd aligned(Qdiff,QR,QL:{{ALIGNMENT}})
    #endif
    for (int n = 0 ; n < {{NUMBER_OF_UNKNOWNS}} ; n++) {
      Qdiff[n] = 0.5 * (QR[xy * {{NUMBER_OF_DATA_PADDED}} + n] - QL[xy * {{NUMBER_OF_DATA_PADDED}} + n]);
    }

{% if USE_NCP %}
    solver.nonconservativeProduct(
      Qavg,
      Qdiff,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      ncp
    );

{% if USE_MATERIAL_PARAMETERS %}
    solver.multiplyMaterialParameterMatrix(
      Qavg,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      ncp
    );
{% endif %}{# USE_MATERIAL_PARAMETERS #}

    #ifdef SharedOMP
    #pragma omp simd aligned(FL,ncp:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
      FL[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += ncp[n];
    }
{% endif %}{# USE_NCP #}

{% if USE_FLUX %}
    solver.flux(
      Qdiff,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      fluxValues
    );

{% if USE_MATERIAL_PARAMETERS %}
    solver.multiplyMaterialParameterMatrix(
      Qavg,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      fluxValues
    );
{% endif %}{# USE_MATERIAL_PARAMETERS #}

    #ifdef SharedOMP
    #pragma omp simd aligned(FL,fluxValues:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FL[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += fluxValues[n];
    }
{% endif %}{# USE_FLUX #}

    #ifdef SharedOMP
    #pragma omp simd aligned(FR,FL,Qdiff:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FR[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] = FL[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] + smax * Qdiff[n];
      FL[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] -= smax * Qdiff[n];
    }
  } // xy
}

template void {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                               solver,
  {{CORRECTOR_COMPUTATION_PRECISION}}* __restrict__             FL,
  {{CORRECTOR_COMPUTATION_PRECISION}}* __restrict__             FR,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const __restrict__ QL,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const __restrict__ QR,
  const double                                                  timeStamp,
  const double                                                  timeStepSize,
  const tarch::la::Vector<Dimensions, double>&                  faceCentre,
  const tarch::la::Vector<Dimensions, double>&                  cellSize,
  const int                                                     direction
);
