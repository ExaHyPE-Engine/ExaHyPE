// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Quadrature.h"

#include <mm_malloc.h>

// Use {{POLYNOMIAL_TYPE}} quadrature

template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<T>::nodes;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<T>::uh2lob;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<T>::dg2fv;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<T>::fv2dg;

template class {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>;

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTE_PRECISION}}>::initQuadratureNodesAndWeights() {
  nodes     = ({{COMPUTE_PRECISION}} *) _mm_malloc(sizeof({{COMPUTE_PRECISION}}) * {{NUMBER_OF_DOF_PADDED}}, {{ALIGNMENT}});
  uh2lob    = ({{COMPUTE_PRECISION}} *) _mm_malloc(sizeof({{COMPUTE_PRECISION}}) * {{uh2lobSize}}, {{ALIGNMENT}}); //nDof*nDofPad
  dg2fv     = ({{COMPUTE_PRECISION}} *) _mm_malloc(sizeof({{COMPUTE_PRECISION}}) * {{dg2fvSize}}, {{ALIGNMENT}}); //nDof*nDofLimPad
  fv2dg     = ({{COMPUTE_PRECISION}} *) _mm_malloc(sizeof({{COMPUTE_PRECISION}}) * {{fv2dgSize}}, {{ALIGNMENT}}); //nDofLim*nDofPad

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
  _mm_free(uh2lob);
  _mm_free(dg2fv);
  _mm_free(fv2dg);
}
