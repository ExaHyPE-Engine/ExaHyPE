// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
{% macro gemm(gemm_key, A, B, C, A_shift, B_shift, C_shift, trueAlpha="", trueB="", force_coeff_matrix=False) %}
{#
      C       = alpha  *   A   *    B   + beta  *  C
   (M x N)              (M x K)  (K x N)
  GEMMS[gemm_key] carries M, N, K, LDA, LDB, LDC, alpha and beta.

  String gemm_key  : key of the entry in GEMMS
  String A         : name of A
  String B         : name of B
  String C         : name of C
  String A_shift   : shift to the first entry of A
  String B_shift   : shift to the first entry of B
  String C_shift   : shift to the first entry of C

  Optional
  String trueB              : array that replaces B; it spans a whole matrix, so a tensor slice
                              does not work
  String trueAlpha          : coefficient that multiplies the alpha of GEMMS, sign included
  bool force_coeff_matrix   : copies trueAlpha*trueB into a temporary coefficient matrix ahead of
                              the loop; it needs trueB and trueAlpha
#}
{% with %}
{% set gemms = GEMMS[gemm_key] %}

{% if trueB is not defined or trueB == "" %}
  {% set trueB = B %}
  {% set useTrueB = False %}
{% else %}
  {% set useTrueB = True %}
{% endif %}

{% if force_coeff_matrix is not defined %}
  {% set force_coeff_matrix = False %}
{% endif %}

{# Strip the index from the array names so the alignment clause names the arrays. #}
{% set Ap = (A.split("["))[0] %}
{% set Bp = (B.split("["))[0] %}
{% set Cp = (C.split("["))[0] %}
{% set trueBp = (trueB.split("["))[0] %}
{% set M = GEMMS[gemm_key].M %}
{% set N = GEMMS[gemm_key].N %}
{% set K = GEMMS[gemm_key].K %}
{% set LDA = GEMMS[gemm_key].LDA %}
{% set LDB = GEMMS[gemm_key].LDB %}
{% set LDC = GEMMS[gemm_key].LDC %}
{% set alpha = GEMMS[gemm_key].alpha %}
{% set beta = GEMMS[gemm_key].beta %}

{# A and C carry unpadded row strides for the face and cell projections, so a
   per-slice base is aligned only when the alignment flag says so. B is loop
   invariant in the vectorised unknown loop, so its alignment never helps and it
   stays out of the clause. #}
{% set aligned_list = [] %}
{% if GEMMS[gemm_key].alignment_C == 1 %}
  {% set aligned_list = aligned_list + [Cp] %}
{% endif %}
{% if GEMMS[gemm_key].alignment_A == 1 %}
  {% set aligned_list = aligned_list + [Ap] %}
{% endif %}
{% set main_aligned = ("aligned(" ~ aligned_list | join(",") ~ ":PEANO_WITH_ALIGNMENT)") if aligned_list else "" %}
{% set reset_aligned = ("aligned(" ~ Cp ~ ":PEANO_WITH_ALIGNMENT)") if GEMMS[gemm_key].alignment_C == 1 else "" %}

{% if force_coeff_matrix %}
  double {{B}}[{{gemms.LDB*gemms.K}}] __attribute__((aligned(PEANO_WITH_ALIGNMENT)));
  SIMD(aligned({{Bp}},{{trueBp}}:PEANO_WITH_ALIGNMENT))
  for (int entry = 0; entry < {{gemms.LDB*gemms.K}}; entry++) {
    {{B}}[entry] = {{trueAlpha}} * {{trueB}}[entry];
  }
  {% set trueB = B %}
{% endif %}

{% if gemms.beta == 0 %}
  for (int column = 0; column < {{gemms.N}}; column++) {
    SIMD({{reset_aligned}})
    for (int unknown = 0; unknown < {{gemms.M}}; unknown++) {
      {{C}}[{{C_shift}} + column * {{gemms.LDC}} + unknown] = 0.0;
    }
  }
{% endif %}

for (int column = 0; column < {{gemms.N}}; column++) {
  for (int row = 0; row < {{gemms.K}}; row++) {
    SIMD({{main_aligned}})
    for (int unknown = 0; unknown < {{gemms.M}}; unknown++) {
      {{C}}[{{C_shift}} + column * {{gemms.LDC}} + unknown] {{ '+' if gemms.alpha == 1 else '-' }}= {{A}}[{{A_shift}} + row * {{gemms.LDA}} + unknown]
        * {% if (useTrueB and not force_coeff_matrix) %}{{trueAlpha}} * {% endif %}{{trueB}}[{% if B_shift != '0' %}{{B_shift}}+{% endif %}column * {{gemms.LDB}} + row];
    }
  }
}
{% endwith %}
{% endmacro %}
