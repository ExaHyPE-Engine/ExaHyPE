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

  inline static T* Kxi = nullptr;
  inline static T* Kxi_T = nullptr;
  inline static T* iK1_T = nullptr; // Note: The generic version of iK1 is actually transposed
  inline static T* dudx = nullptr;
  inline static T* dudx_T = nullptr;
  inline static T* FLCoeff = nullptr;
  inline static T* FRCoeff = nullptr;
  inline static T** fineGridProjector1d = nullptr;
  inline static T** fineGridProjector1d_T_weighted = nullptr; // [k][i*nDof+j] = fineGridProjector1d[k][j*nDof+i] * weight[j] / weight[i] / 3.0
};

{% for item in NAMESPACE -%}
}
{%- endfor %}
