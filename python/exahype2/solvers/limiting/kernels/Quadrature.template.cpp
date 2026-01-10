// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Quadrature.h"

#include "tarch/accelerator/accelerator.h"

// Using {{POLYNOMIAL_TYPE}} quadrature

template class {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>;

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::initQuadratureNodesAndWeights() {
  nodes   = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{NUMBER_OF_DOF_PADDED}}, tarch::MemoryLocation::Heap);
  uh2lob  = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{uh2lobSize}}, tarch::MemoryLocation::Heap); // nDof*nDofPad
  dg2fv   = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{dg2fvSize}}, tarch::MemoryLocation::Heap); // nDof*nDofLimPad
  fv2dg   = tarch::allocateMemory<{{COMPUTE_PRECISION}}>({{fv2dgSize}}, tarch::MemoryLocation::Heap); // nDofLim*nDofPad

{% for i in quadrature_seq %}
  nodes[{{i}}]     = ({{COMPUTE_PRECISION}}) {{"{:.15e}".format(QuadratureNodes[i])}};
{% endfor %}

{% for i in uh2lob_seq %}
  uh2lob[{{i}}]    = ({{COMPUTE_PRECISION}}) {{"{:.15e}".format(uh2lob[i])}};
{% endfor %}

{% for i in dg2fv_seq %}
  dg2fv[{{i}}]    = ({{COMPUTE_PRECISION}}) {{"{:.15e}".format(dg2fv[i])}};
{% endfor %}

{% for i in fv2dg_seq %}
  fv2dg[{{i}}]    = ({{COMPUTE_PRECISION}}) {{"{:.15e}".format(fv2dg[i])}};
{% endfor %}
}

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::freeQuadratureNodesAndWeights() {
  tarch::freeMemory(nodes, tarch::MemoryLocation::Heap);
  tarch::freeMemory(uh2lob, tarch::MemoryLocation::Heap);
  tarch::freeMemory(dg2fv, tarch::MemoryLocation::Heap);
  tarch::freeMemory(fv2dg, tarch::MemoryLocation::Heap);
}
