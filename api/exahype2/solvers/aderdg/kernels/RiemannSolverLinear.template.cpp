// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "DGMatrices.h"
#include "Quadrature.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/instrumentation/Trace.h"

#include <algorithm>
#include <cstring>
#include <cmath>

template <typename cCompType>
void {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  cCompType* NOALIAS                                FL,
  cCompType* NOALIAS                                FR,
  const cCompType* NOALIAS const                    QL,
  const cCompType* NOALIAS const                    QR,
  const double                                      timeStamp,
  const double                                      timeStepSize,
  const tarch::la::Vector<DIMENSIONS, double>&      faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      cellSize,
  const int                                         direction,
  const bool                                        isBoundary,
  const int                                         faceNumber
) {
  traceEnter();

{% if USE_RIEMANN_SOLVER %}
  traceEnter("User Riemann solver");
{% if STATELESS_RIEMANN_SOLVER %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::riemannSolver(
{% else %}
  solver.riemannSolver(
{% endif %}
    FL,
    FR,
    QL,
    QR,
    timeStamp,
    timeStepSize,
    faceCentre,
    cellSize,
    direction,
    isBoundary,
    faceNumber
  );
  traceLeave("User Riemann solver");
{% else %}
  /*
   * For the linear kernels, we need the material parameters in the
   * extrapolated predictor.
   * We compute the averages of the material parameters but
   * do not use them in the max eigenvalue calculation.
   */
  std::memset(FL, 0, {{NUMBER_OF_UNKNOWNS * NUMBER_OF_DOF * NUMBER_OF_DOF_3D}} * sizeof(cCompType));
  std::memset(FR, 0, {{NUMBER_OF_UNKNOWNS * NUMBER_OF_DOF * NUMBER_OF_DOF_3D}} * sizeof(cCompType));

  // Compute the average unkown and auxiliary variables from the left and the right
  cCompType QavL[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};
  cCompType QavR[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};

  traceEnter("Average states");
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    SIMD(aligned(QavL,QavR,QL:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_DATA}}; n++) {
      QavL[n] += Quadrature<cCompType>::weights2[xy] * QL[xy * {{NUMBER_OF_DATA}} + n];
      QavR[n] += Quadrature<cCompType>::weights2[xy] * QR[xy * {{NUMBER_OF_DATA}} + n];
    }
  }
  traceLeave("Average states");

  traceEnter("Max eigenvalue evaluation");
{% if STATELESS_MAX_EIGENVALUE %}
  cCompType smaxL = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(
{% else %}
  cCompType smaxL = solver.maxEigenvalue(
{% endif %}
    &QavL[0],
    faceCentre,
    cellSize,
    timeStamp,
    timeStepSize,
    direction
  );
{% if STATELESS_MAX_EIGENVALUE %}
  cCompType smaxR = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(
{% else %}
  cCompType smaxR = solver.maxEigenvalue(
{% endif %}
    &QavR[0],
    faceCentre,
    cellSize,
    timeStamp,
    timeStepSize,
    direction
  );

  cCompType smax = std::fmax(smaxL, smaxR);
  traceLeave("Max eigenvalue evaluation");

{% if USE_NCP or USE_MATERIAL_PARAMETERS or NUMBER_OF_AUXILIARY_VARIABLES > 0 %}
  cCompType Qavg[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

  SIMD(aligned(Qavg,QavR,QavL:PEANO_WITH_ALIGNMENT))
  for (int n = 0; n < {{NUMBER_OF_DATA_PADDED}}; n++) {
    Qavg[n] = 0.5 * (QavR[n] + QavL[n]);
  }
{% endif %}

  cCompType Qdiff[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));

{% if NUMBER_OF_AUXILIARY_VARIABLES > 0 %}
  // We copy the averaged material parameters to Qdiff as they are used in the flux term
  std::copy_n(Qavg + {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_DATA_PADDED - NUMBER_OF_UNKNOWNS}}, Qdiff + {{NUMBER_OF_UNKNOWNS}});
{% endif %}

{% if USE_NCP %}
  cCompType ncp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};
{% endif %}

{% if USE_FLUX %}
  cCompType fluxValues[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT))) = {0.0};
{% endif %}

  traceEnter("Flux and nonconservative product evaluation");
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    SIMD(aligned(Qdiff,QL:PEANO_WITH_ALIGNMENT))
    for (int n = 0 ; n < {{NUMBER_OF_UNKNOWNS}} ; n++) {
      Qdiff[n] = 0.5 * (QR[xy * {{NUMBER_OF_DATA}} + n] - QL[xy * {{NUMBER_OF_DATA}} + n]);
    }

{% if USE_NCP %}
{% if STATELESS_NCP %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(
{% else %}
    solver.nonconservativeProduct(
{% endif %}
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
{% if STATELESS_MATERIAL_PARAMETERS %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::multiplyMaterialParameterMatrix(
{% else %}
    solver.multiplyMaterialParameterMatrix(
{% endif %}
      Qavg,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      ncp
    );
{% endif %}{# USE_MATERIAL_PARAMETERS #}

    SIMD(aligned(FL,ncp:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FL[xy * {{NUMBER_OF_UNKNOWNS}} + n] += ncp[n];
    }
{% endif %}{# USE_NCP #}

{% if USE_FLUX %}
{% if STATELESS_FLUX %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::flux(
{% else %}
    solver.flux(
{% endif %}
      Qdiff,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      fluxValues
    );

{% if USE_MATERIAL_PARAMETERS %}
{% if STATELESS_MATERIAL_PARAMETERS %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::multiplyMaterialParameterMatrix(
{% else %}
    solver.multiplyMaterialParameterMatrix(
{% endif %}
      Qavg,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      fluxValues
    );
{% endif %}{# USE_MATERIAL_PARAMETERS #}

    SIMD(aligned(FL,fluxValues:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FL[xy * {{NUMBER_OF_UNKNOWNS}} + n] += fluxValues[n];
    }
{% endif %}{# USE_FLUX #}

    SIMD(aligned(FL,Qdiff:PEANO_WITH_ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FR[xy * {{NUMBER_OF_UNKNOWNS}} + n] = FL[xy * {{NUMBER_OF_UNKNOWNS}} + n] + smax * Qdiff[n];
      FL[xy * {{NUMBER_OF_UNKNOWNS}} + n] -= smax * Qdiff[n];
    }
  } // xy
  traceLeave("Flux and nonconservative product evaluation");
{% endif %}
  traceLeave();
}

template void {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                           solver,
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS              FL,
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS              FR,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  QL,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  QR,
  const double                                              timeStamp,
  const double                                              timeStepSize,
  const tarch::la::Vector<DIMENSIONS, double>&              faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>&              cellSize,
  const int                                                 direction,
  const bool                                                isBoundary,
  const int                                                 faceNumber
);
