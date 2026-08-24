// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGMatrices.h"

#include "tarch/accelerator/accelerator.h"

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template <>
void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initDGMatrices() {
  FLCoeff = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED}}, tarch::MemoryLocation::Heap);
  FRCoeff = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED}}, tarch::MemoryLocation::Heap);
  // Note: FLCoeff is also F0

  dudx   = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  dudx_T = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  iK1_T  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  Kxi    = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);
  Kxi_T  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, tarch::MemoryLocation::Heap);

{% for i in nDofPad_seq %}
  FLCoeff[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(FLCoeff[i])}};
{% endfor %}

{% for i in nDofPad_seq %}
  FRCoeff[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(FRCoeff[i])}};
{% endfor %}

{% for i in nDofTimesnDof_seq %}
  dudx[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(dudx[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  dudx_T[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(dudx_T[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  iK1_T[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(iK1_T[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  Kxi[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(Kxi[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  Kxi_T[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(Kxi_T[i])}};
{% endfor %}
}

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeDGMatrices() {
  tarch::freeMemory(FLCoeff, tarch::MemoryLocation::Heap);
  tarch::freeMemory(FRCoeff, tarch::MemoryLocation::Heap);
  tarch::freeMemory(dudx, tarch::MemoryLocation::Heap);
  tarch::freeMemory(dudx_T, tarch::MemoryLocation::Heap);
  tarch::freeMemory(iK1_T, tarch::MemoryLocation::Heap);
  tarch::freeMemory(Kxi, tarch::MemoryLocation::Heap);
  tarch::freeMemory(Kxi_T, tarch::MemoryLocation::Heap);
}
{% endfor %}
