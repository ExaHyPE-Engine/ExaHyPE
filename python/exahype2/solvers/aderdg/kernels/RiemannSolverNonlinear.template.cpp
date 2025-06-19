// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "DGMatrices.h"
#include "Quadrature.h"

#include <algorithm>
#include <cstring>
#include <cmath>

#include <mm_malloc.h>

template <typename cCompType>
void {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  cCompType* NOALIAS                                FL,
  cCompType* NOALIAS                                FR,
  const cCompType* NOALIAS const                    QL,
  const cCompType* NOALIAS const                    QR,
  const double                                      timeStamp,
  const double                                      timeStepSize,
  const tarch::la::Vector<Dimensions, double>&      faceCentre,
  const tarch::la::Vector<Dimensions, double>&      cellSize,
  const int                                         direction,
  const bool                                        isBoundary,
  const int                                         faceNumber
) {
{% if USE_RIEMANN_SOLVER %}
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
{% else %}
  cCompType* tmp_bnd = static_cast<cCompType *>(_mm_malloc({{NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}} * sizeof(cCompType), {{ALIGNMENT}}));
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

  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    #ifdef SharedOMP
    #pragma omp simd aligned(tmp_bnd,QL,QR:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      tmp_bnd[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] = smax * (QL[xy * {{NUMBER_OF_DATA_PADDED}} + n] - QR[xy * {{NUMBER_OF_DATA_PADDED}} + n]);
    }
  }

  #ifdef SharedOMP
  #pragma omp simd aligned(tmp_bnd:{{ALIGNMENT}})
  #endif
  for (int xyn = 0; xyn < {{NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xyn++) {
    FL[xyn] = 0.5 * (FL[xyn] + FR[xyn] + tmp_bnd[xyn]);
  }
  std::copy_n(FL, {{NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}, FR);
  _mm_free(tmp_bnd);

{% if USE_NCP %}
  cCompType Qavg[{{NUMBER_OF_DATA}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
  cCompType gradQ[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};
  cCompType ncp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned({{ALIGNMENT}}))) = {0.0};

  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    #ifdef SharedOMP
    #pragma omp simd aligned(Qavg,QL,QR,gradQ:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
       Qavg[n] = 0.5 * (QL[xy * {{NUMBER_OF_DATA_PADDED}} + n] + QR[xy * {{NUMBER_OF_DATA_PADDED}} + n]);
       gradQ[n] = QR[xy * {{NUMBER_OF_DATA_PADDED}} + n] - QL[xy * {{NUMBER_OF_DATA_PADDED}} + n];
    }
{% if NUMBER_OF_AUXILIARY_VARIABLES != 0 %}
    for (int n = {{NUMBER_OF_UNKNOWNS}}; n < {{NUMBER_OF_DATA}}; n++) {
      Qavg[n] = 0.5 * (QL[xy * {{NUMBER_OF_DATA_PADDED}} + n] + QR[xy * {{NUMBER_OF_DATA_PADDED}} + n]);
    }
{% endif %}
{% if STATELESS_NCP %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(
{% else %}
    solver.nonconservativeProduct(
{% endif %}
      Qavg,
      gradQ,
      faceCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      direction,
      ncp
    );

    #ifdef SharedOMP
    #pragma omp simd aligned(FL,FR,ncp:{{ALIGNMENT}})
    #endif
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS_PADDED}}; n++) {
      FR[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] -= 0.5 * ncp[n];
      FL[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] += 0.5 * ncp[n];
    }
  }
{% endif %}
{% endif %}
}

template void {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                           solver,
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS              FL,
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS              FR,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  QL,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS  QR,
  const double                                              timeStamp,
  const double                                              timeStepSize,
  const tarch::la::Vector<Dimensions, double>&              faceCentre,
  const tarch::la::Vector<Dimensions, double>&              cellSize,
  const int                                                 direction,
  const bool                                                isBoundary,
  const int                                                 faceNumber
);
