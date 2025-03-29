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
  static T *weights1;
  static T *weights2;
  static T *weights3;
  static T *weights4;
  static T *iweights1;
  static T *iweights3;
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
