// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

/**
 * 1D quadrature of the solver's Lagrange basis.
 *
 * The arrays are filled by initQuadratureNodesAndWeights(), which the solver's
 * startSimulation() calls before the first grid traversal.
 */
class Quadrature final {
public:
  static void initQuadratureNodesAndWeights();
  static void freeQuadratureNodesAndWeights();
{% for vector in QUADRATURE %}
  /**
{{ vector.documentation }}   */
  inline static double* {{vector.name}} = nullptr;
{% endfor %}};

{% for item in NAMESPACE -%}
}
{%- endfor %}
