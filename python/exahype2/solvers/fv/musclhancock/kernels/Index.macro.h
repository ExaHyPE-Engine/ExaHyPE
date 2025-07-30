// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro index(volume, unknown) %}
  {% if DIMENSIONS==2 %}
  ({{volume}}(1)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PER_AXIS}} +
  ({{volume}}(0)) * {{NUMBER_OF_VARIABLES_IN_VOLUME}} +
  {{unknown}};
  {% else %}
  ({{volume}}(2)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} +
  ({{volume}}(1)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PER_AXIS}} +
  ({{volume}}(0)) * {{NUMBER_OF_VARIABLES_IN_VOLUME}} +
  {{unknown}};
  {% endif %}
{% endmacro %}

{% macro indexWithHalo(volume, unknown) %}
  {% if DIMENSIONS==2 %}
  ({{volume}}(1) + {{HALO_SIZE}}) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_PER_AXIS}} +
  ({{volume}}(0) + {{HALO_SIZE}}) * {{NUMBER_OF_VARIABLES_IN_VOLUME}} +
  {{unknown}};
  {% else %}
  ({{volume}}(2) + {{HALO_SIZE}}) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} +
  ({{volume}}(1) + {{HALO_SIZE}}) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_PER_AXIS}} +
  ({{volume}}(0) + {{HALO_SIZE}}) * {{NUMBER_OF_VARIABLES_IN_VOLUME}} +
  {{unknown}};
  {% endif %}
{% endmacro %}
