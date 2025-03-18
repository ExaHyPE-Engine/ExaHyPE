// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGMatrices.h"

#include <mm_malloc.h>

template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::Kxi;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::Kxi_T;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::iK1_T;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::dudx;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::dudx_T;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::FLCoeff;
template <typename T> T* {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::FRCoeff;
template <typename T> T** {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::fineGridProjector1d;
template <typename T> T** {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<T>::fineGridProjector1d_T_weighted;

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template class {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>;
{% endfor %}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template <>
void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initDGMatrices() {
  FLCoeff = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED}}, {{ALIGNMENT}});
  FRCoeff = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED}}, {{ALIGNMENT}});
  // Note: FLCoeff is also F0

  dudx    = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});
  dudx_T  = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});
  iK1_T   = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});
  Kxi     = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});
  Kxi_T   = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});

  fineGridProjector1d            = new {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* [3];
  fineGridProjector1d_T_weighted = new {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* [3];
  for (int i = 0; i < 3; i++) {
    fineGridProjector1d[i]            = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});
    fineGridProjector1d_T_weighted[i] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *) _mm_malloc(sizeof({{COMPUTATION_PRECISIONS[PRECISION_NUM]}})*{{NUMBER_OF_DOF_PADDED*NUMBER_OF_DOF}}, {{ALIGNMENT}});
  }

{% for i in nDofPad_seq %}
  FLCoeff[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(FLCoeff[i])}};
{% endfor %}

{% for i in nDofPad_seq %}
  FRCoeff[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(FRCoeff[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  dudx[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(dudx[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  dudx_T[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(dudx_T[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  iK1_T[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(iK1_T[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  Kxi[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(Kxi[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  Kxi_T[{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(Kxi_T[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  fineGridProjector1d[0][{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(fineGridProjector1d_0[i])}};
{% endfor %}
{% for i in nDofPadTimesnDof_seq %}
  fineGridProjector1d[1][{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(fineGridProjector1d_1[i])}};
{% endfor %}
{% for i in nDofPadTimesnDof_seq %}
  fineGridProjector1d[2][{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(fineGridProjector1d_2[i])}};
{% endfor %}

{% for i in nDofPadTimesnDof_seq %}
  fineGridProjector1d_T_weighted[0][{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(fineGridProjector1d_T_weighted_0[i])}};
{% endfor %}
{% for i in nDofPadTimesnDof_seq %}
  fineGridProjector1d_T_weighted[1][{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(fineGridProjector1d_T_weighted_1[i])}};
{% endfor %}
{% for i in nDofPadTimesnDof_seq %}
  fineGridProjector1d_T_weighted[2][{{i}}] = ({{COMPUTATION_PRECISIONS[PRECISION_NUM]}}) {{"{:.15e}".format(fineGridProjector1d_T_weighted_2[i])}};
{% endfor %}
}

template <>
void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeDGMatrices() {
  _mm_free(FLCoeff);
  _mm_free(FRCoeff);
  _mm_free(dudx);
  _mm_free(dudx_T);
  _mm_free(iK1_T);
  _mm_free(Kxi);
  _mm_free(Kxi_T);

  _mm_free(fineGridProjector1d[0]);
  _mm_free(fineGridProjector1d[1]);
  _mm_free(fineGridProjector1d[2]);
  delete [] fineGridProjector1d;

  _mm_free(fineGridProjector1d_T_weighted[0]);
  _mm_free(fineGridProjector1d_T_weighted[1]);
  _mm_free(fineGridProjector1d_T_weighted[2]);
  delete [] fineGridProjector1d_T_weighted;
}
{% endfor %}
