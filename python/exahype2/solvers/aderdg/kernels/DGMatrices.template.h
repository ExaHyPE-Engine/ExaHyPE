// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

// All matrices are stored as column major array, _T denote a transposed.
template <typename T>
class DGMatrices {
public:
  static void initDGMatrices();
  static void freeDGMatrices();

  static T* Kxi;
  static T* Kxi_T;
  static T* iK1_T; // Note: The generic version of iK1 is actually transposed
  static T* dudx;
  static T* dudx_T;
  static T* FLCoeff;
  static T* FRCoeff;
  static T** fineGridProjector1d;
  static T** fineGridProjector1d_T_weighted; // [k][i*nDof+j] = fineGridProjector1d[k][j*nDof+i] * weight[j] / weight[i] / 3.0
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
