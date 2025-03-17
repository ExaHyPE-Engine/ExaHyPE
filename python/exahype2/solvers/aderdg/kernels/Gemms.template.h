// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

{% if USE_LIBXSMM %}

{% for gemm,precision in GEMM_LIST %}
{% if precision == "DP" %}
void {{gemm}}(const double* A, const double* B, double* C);
{% elif precision == "SP"%}
void {{gemm}}(const float* A, const float* B, float* C);
{% else %}
void {{gemm}}(const long double* A, const long double* B, long double* C);
{% endif %}
{% endfor %}

{% endif %}{# USE_LIBXSMM #}

{% for item in NAMESPACE -%}
}
{%- endfor %}

{% macro gemm(gemm_key, A, B, C, A_shift, B_shift, C_shift, true_alpha="", true_B="", force_coeff_matrix=False) %}
{#
      C       = alpha  *   A   *    B   + beta  *  C
   (M x N)              (M x K)  (K x N)
  The gemms (fetched through gemm_key) contains M, N, K, LDA, LDB, LDC, alpha and beta

  String gemm_key  : name of the associated GEMMS
  String A         : name of A
  String B         : name of B
  String C         : name of C
  String A_shift   : shift to the zero of A
  String B_shift   : shift to the zero of B
  String C_shift   : shift to the zero of C

  optional
  String true_B             : true array B, B must b a true matrix, not a tensor slice
  String true_alpha         : true value of the coefficent alpha (note: it will be multiplied by the GEMMS alpha, /!\ sign error)
  bool force_coeff_matrix   : only when using true_B, true_alpha, force the no libxsmm case to also generate the coeff matrix.

  If true_B is used, a temporary array true_alpha*true_B is generated
#}
{% with %}
{% set gemms = GEMMS[gemm_key] %}

{% if true_B is not defined or true_B == "" %}
  {% set true_B = B %}
  {% set use_true_B = False %}
{% else %}
  {% set use_true_B = True %}
{% endif %}

{% if force_coeff_matrix is not defined %}
  {% set force_coeff_matrix = False %}
{% endif %}

{# set arrays' name for pragma by removing eventual index #}
{% set Ap = (A.split("["))[0] %}
{% set Bp = (B.split("["))[0] %}
{% set Cp = (C.split("["))[0] %}
{% set true_Bp = (true_B.split("["))[0] %}
{% set M = GEMMS[gemm_key].M %}
{% set N = GEMMS[gemm_key].N %}
{% set K = GEMMS[gemm_key].K %}
{% set LDA = GEMMS[gemm_key].LDA %}
{% set LDB = GEMMS[gemm_key].LDB %}
{% set LDC = GEMMS[gemm_key].LDC %}
{% set alpha = GEMMS[gemm_key].alpha %}
{% set beta = GEMMS[gemm_key].beta %}

{% set precision = GEMMS[gemm_key].precision[0] %}
{% if precision == "DP" %}
  {% set precision_prefix = "d" %}
  {% set precision_type = "double" %}
{% elif precision == "SP" %}
  {% set precision_prefix = "s" %}
  {% set precision_type = "float" %}
{% endif %}

{% if GEMMS[gemm_key].alignment_A == 1 %}
  {% set alignment_A = "Aligned" %}
{% else %}
  {% set alignment_A = "Unaligned" %}
{% endif %}

{% if GEMMS[gemm_key].alignment_C == 1 %}
  {% set alignment_C = "Aligned" %}
{% else %}
  {% set alignment_C = "Unaligned" %}
{% endif %}

{% if USE_LIBXSMM %}
  {{gemms.baseroutinename}}({{A}}{% if A_shift != '0' %}+{{A_shift}}{% endif %}, {{B}}{% if B_shift != '0' %}+{{B_shift}}{% endif %}, {{C}}{% if C_shift != '0' %}+{{C_shift}}{% endif %});
{% else %}
{% if force_coeff_matrix %}
  double {{B}}[{{gemms.LDB*gemms.K}}] __attribute__((aligned({{ALIGNMENT}})));
  #ifdef SharedOMP
  #pragma omp simd aligned({{Bp}},{{true_Bp}}:{{ALIGNMENT}})
  #endif
  for (int it = 0; it < {{gemms.LDB*gemms.K}}; it++) {
    {{B}}[it] = {{true_alpha}} * {{true_B}}[it];
  }
  {% set true_B = B %}
{% endif %}

{% if gemms.beta == 0 %}
  // Reset {{C}}
  for (int it_1 = 0; it_1 < {{gemms.N}}; it_1++) {
    #ifdef SharedOMP
    #pragma omp simd aligned({{Cp}}:{{ALIGNMENT}})
    #endif
    for (int it_3 = 0; it_3 < {{gemms.M}}; it_3++) {
      {{C}}[{{C_shift}} + it_1 * {{gemms.LDC}} + it_3] = 0.0;
    }
  }
{% endif %}

for (int it_1 = 0; it_1 < {{gemms.N}}; it_1++) {
  for (int it_2 = 0; it_2 < {{gemms.K}}; it_2++) {
    #ifdef SharedOMP
    #pragma omp simd aligned({{Cp}},{{Ap}},{{true_Bp}}:{{ALIGNMENT}})
    #endif
    for (int it_3 = 0; it_3 < {{gemms.M}}; it_3++) {
      {{C}}[{{C_shift}} + it_1 * {{gemms.LDC}} + it_3] {{ '+' if gemms.alpha == 1 else '-' }}= {{A}}[{{A_shift}} + it_2 * {{gemms.LDA}} + it_3]
        * {% if (use_true_B and not force_coeff_matrix) %}{{true_alpha}} * {% endif %}{{true_B}}[{% if B_shift != '0' %}{{B_shift}}+{% endif %}it_1 * {{gemms.LDB}} + it_2];
    }
  }
}
{% endif %} {# USE_LIBXSMM #}
{% endwith %}
{% endmacro %}
{#
