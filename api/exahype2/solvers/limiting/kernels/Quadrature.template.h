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

  inline static T *nodes = nullptr;
  // Limiter projection matrices
  inline static T* uh2lob = nullptr;
  inline static T* dg2fv = nullptr;
  inline static T* fv2dg = nullptr;
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
