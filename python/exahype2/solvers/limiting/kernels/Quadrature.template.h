// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

template <typename T>
class Quadrature {
public:
  static void initQuadratureNodesAndWeights();
  static void freeQuadratureNodesAndWeights();

  static T *nodes;
  // Limiter projection matrices
  static T* uh2lob;
  static T* dg2fv;
  static T* fv2dg;
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
