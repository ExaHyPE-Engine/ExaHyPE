// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Quadrature.h"

#include "tarch/accelerator/accelerator.h"

// Using {{POLYNOMIAL_TYPE}} quadrature

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template <>
void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initQuadratureNodesAndWeights() {
  nodes     = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{NUMBER_OF_DOF_PADDED}}, tarch::MemoryLocation::Heap);
  weights1  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{w1Size}}, tarch::MemoryLocation::Heap); // nDofPad
  weights2  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{w2Size}}, tarch::MemoryLocation::Heap); // 2D: nDofPad (==weight1), 3D: (nDof*nDof)Pad (== w1[i]*w1[j])
  weights3  = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{w3Size}}, tarch::MemoryLocation::Heap); // 2D: (nDof*nDof)Pad (== w1[i]*w1[j]), 3D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k])
  iweights1 = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{w1Size}}, tarch::MemoryLocation::Heap); // nDofPad
  iweights3 = tarch::allocateMemory<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>({{w3Size}}, tarch::MemoryLocation::Heap); // 2D: (nDof*nDof)Pad (== w1[i]*w1[j]), 3D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k])

{% for i in quadrature_seq %}
  nodes[{{i}}]     = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(QuadratureNodes[i])}};
{% endfor %}

{% for i in w1_seq %}
  weights1[{{i}}]  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(weights1[i])}};
{% endfor %}

{% for i in w2_seq %}
  weights2[{{i}}]  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(weights2[i])}};
{% endfor %}

{% for i in w3_seq %}
  weights3[{{i}}]  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(weights3[i])}};
{% endfor %}

{% for i in w1_seq %}
  iweights1[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(iweights1[i])}};
{% endfor %}

{% for i in w3_seq %}
  iweights3[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(iweights3[i])}};
{% endfor %}
}

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeQuadratureNodesAndWeights() {
  tarch::freeMemory(nodes, tarch::MemoryLocation::Heap);
  tarch::freeMemory(weights1, tarch::MemoryLocation::Heap);
  tarch::freeMemory(weights2, tarch::MemoryLocation::Heap);
  tarch::freeMemory(weights3, tarch::MemoryLocation::Heap);
  tarch::freeMemory(iweights1, tarch::MemoryLocation::Heap);
  tarch::freeMemory(iweights3, tarch::MemoryLocation::Heap);
}
{% endfor %}
