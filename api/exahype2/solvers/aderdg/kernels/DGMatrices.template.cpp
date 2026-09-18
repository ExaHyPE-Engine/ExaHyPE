// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGMatrices.h"

#include "tarch/accelerator/accelerator.h"

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
/**
 * Allocates the matrices and fills them with the tabulated entries.
 */
template <>
void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initDGMatrices() {
  FLCoeff = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED}}, tarch::MemoryLocation::Heap);
  FRCoeff = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED}}, tarch::MemoryLocation::Heap);
  // FLCoeff carries the coefficients that the ADER-DG literature names F0.

  dudx           = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  dudxTransposed = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  iK1Transposed  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  Kxi            = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  KxiTransposed  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);

{% for i in nDofPadRange %}
  FLCoeff[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(FLCoeff[i])}});
{% endfor %}

{% for i in nDofPadRange %}
  FRCoeff[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(FRCoeff[i])}});
{% endfor %}

{% for i in nDofTimesnDofRange %}
  dudx[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(dudx[i])}});
{% endfor %}

{% for i in nDofPadTimesnDofRange %}
  dudxTransposed[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(dudxTransposed[i])}});
{% endfor %}

{% for i in nDofPadTimesnDofRange %}
  iK1Transposed[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(iK1Transposed[i])}});
{% endfor %}

{% for i in nDofPadTimesnDofRange %}
  Kxi[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(Kxi[i])}});
{% endfor %}

{% for i in nDofPadTimesnDofRange %}
  KxiTransposed[{{i}}] = static_cast<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{"{:.15e}".format(KxiTransposed[i])}});
{% endfor %}
}

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeDGMatrices() {
  tarch::freeMemory(FLCoeff, tarch::MemoryLocation::Heap);
  tarch::freeMemory(FRCoeff, tarch::MemoryLocation::Heap);
  tarch::freeMemory(dudx, tarch::MemoryLocation::Heap);
  tarch::freeMemory(dudxTransposed, tarch::MemoryLocation::Heap);
  tarch::freeMemory(iK1Transposed, tarch::MemoryLocation::Heap);
  tarch::freeMemory(Kxi, tarch::MemoryLocation::Heap);
  tarch::freeMemory(KxiTransposed, tarch::MemoryLocation::Heap);
}
{% endfor %}
