// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Quadrature.h"

#include <mm_malloc.h>

// Using {{POLYNOMIAL_TYPE}} quadrature

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template <>
void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initQuadratureNodesAndWeights() {
  nodes     = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) * {{NUMBER_OF_DOF_PADDED}}, {{ALIGNMENT}});
  weights1  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) * {{w1Size}}, {{ALIGNMENT}}); // nDofPad
  weights2  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) * {{w2Size}}, {{ALIGNMENT}}); // 2D: nDofPad (==weight1), 3D: (nDof*nDof)Pad (== w1[i]*w1[j])
  weights3  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) * {{w3Size}}, {{ALIGNMENT}}); // 2D: (nDof*nDof)Pad (== w1[i]*w1[j]), 3D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k])
  iweights1 = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) * {{w1Size}}, {{ALIGNMENT}}); // nDofPad
  iweights3 = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) * {{w3Size}}, {{ALIGNMENT}}); // 2D: (nDof*nDof)Pad (== w1[i]*w1[j]), 3D: (nDof*nDof*nDof)Pad (== w1[i]*w1[j]*w1[k])

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
  _mm_free(nodes);
  _mm_free(weights1);
  _mm_free(weights2);
  _mm_free(weights3);
  _mm_free(iweights1);
  _mm_free(iweights3);
}
{% endfor %}
