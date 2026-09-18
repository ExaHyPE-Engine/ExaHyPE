// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

/**
 * 1D reference operators of the solver's Lagrange basis.
 *
 * All matrices are flattened row major, the inter-grid transfer operators as
 * one consecutive block per child interval. The arrays are filled by
 * initDGMatrices(), which the solver's startSimulation() calls before the
 * first grid traversal.
 */
class DGMatrices final {
public:
  static void initDGMatrices();
  static void freeDGMatrices();
{% for matrix in DG_MATRICES %}
  /**
{{ matrix.documentation }}   */
  inline static double* {{matrix.name}} = nullptr;
{% endfor %}};

{% for item in NAMESPACE -%}
}
{%- endfor %}
