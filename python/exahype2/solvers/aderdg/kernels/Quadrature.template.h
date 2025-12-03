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
  inline static T *weights1 = nullptr;
  inline static T *weights2 = nullptr;
  inline static T *weights3 = nullptr;
  inline static T *iweights1 = nullptr;
  inline static T *iweights3 = nullptr;
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
