// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "DGMatrices.h"
#include "Quadrature.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
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
  cCompType* tmp_bnd = tarch::allocateMemory<cCompType>({{NUMBER_OF_UNKNOWNS_PADDED*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}, tarch::MemoryLocation::Heap);
  cCompType QavL[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned(ALIGNMENT))) = {0.0};
  cCompType QavR[{{NUMBER_OF_DATA_PADDED}}] __attribute__((aligned(ALIGNMENT))) = {0.0};

  traceEnter("Average states");
  // TODO: This loop still iterates over NUMBER_OF_DATA_PADDED while QL/QR are
  // indexed with the unpadded NUMBER_OF_DATA stride below. A padded build can
  // therefore read past one face slice unless storage and loop bounds match.
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    SIMD(aligned(QavL,QavR,QL:ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_DATA_PADDED}}; n++) {
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

  traceEnter("Rusanov flux");
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    SIMD(aligned(tmp_bnd,QL:ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      tmp_bnd[xy * {{NUMBER_OF_UNKNOWNS_PADDED}} + n] = smax * (QL[xy * {{NUMBER_OF_DATA}} + n] - QR[xy * {{NUMBER_OF_DATA}} + n]);
    }
  }

  SIMD(aligned(FL,tmp_bnd:ALIGNMENT))
  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FL[xy*{{NUMBER_OF_UNKNOWNS}} + n] = 0.5 * (FL[xy*{{NUMBER_OF_UNKNOWNS}} + n] + FR[xy*{{NUMBER_OF_UNKNOWNS}} + n] + tmp_bnd[xy*{{NUMBER_OF_UNKNOWNS_PADDED}} + n]);
    }
  }
  std::copy_n(FL, {{NUMBER_OF_UNKNOWNS*NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}, FR);
  tarch::freeMemory(tmp_bnd, tarch::MemoryLocation::Heap);
  traceLeave("Rusanov flux");

{% if USE_NCP %}
  traceEnter("Nonconservative product evaluation");
  cCompType Qavg[{{NUMBER_OF_DATA}}] __attribute__((aligned(ALIGNMENT))) = {0.0};
  cCompType gradQ[{{NUMBER_OF_UNKNOWNS}}] __attribute__((aligned(ALIGNMENT))) = {0.0};
  cCompType ncp[{{NUMBER_OF_UNKNOWNS_PADDED}}] __attribute__((aligned(ALIGNMENT))) = {0.0};

  for (int xy = 0; xy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; xy++) {
    SIMD(aligned(Qavg,QL,gradQ:ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
       Qavg[n] = 0.5 * (QL[xy * {{NUMBER_OF_DATA}} + n] + QR[xy * {{NUMBER_OF_DATA}} + n]);
       gradQ[n] = QR[xy * {{NUMBER_OF_DATA}} + n] - QL[xy * {{NUMBER_OF_DATA}} + n];
    }
{% if NUMBER_OF_AUXILIARY_VARIABLES != 0 %}
    for (int n = {{NUMBER_OF_UNKNOWNS}}; n < {{NUMBER_OF_DATA}}; n++) {
      Qavg[n] = 0.5 * (QL[xy * {{NUMBER_OF_DATA}} + n] + QR[xy * {{NUMBER_OF_DATA}} + n]);
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

    SIMD(aligned(FL,ncp:ALIGNMENT))
    for (int n = 0; n < {{NUMBER_OF_UNKNOWNS}}; n++) {
      FR[xy * {{NUMBER_OF_UNKNOWNS}} + n] -= 0.5 * ncp[n];
      FL[xy * {{NUMBER_OF_UNKNOWNS}} + n] += 0.5 * ncp[n];
    }
  }
  traceLeave("Nonconservative product evaluation");
{% endif %}
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
