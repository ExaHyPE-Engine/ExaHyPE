// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro index(node, unknown) %}
  {% if DIMENSIONS==2 %}
  ({{node}}(1)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PER_AXIS}} +
  ({{node}}(0)) * {{NUMBER_OF_VARIABLES_AT_NODE}} +
  {{unknown}};
  {% else %}
  ({{node}}(2)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} +
  ({{node}}(1)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PER_AXIS}} +
  ({{node}}(0)) * {{NUMBER_OF_VARIABLES_AT_NODE}} +
  {{unknown}};
  {% endif %}
{% endmacro %}

{% macro indexWithHalo(node, unknown) %}
  {% if DIMENSIONS==2 %}
  ({{node}}(1) + {{HALO_SIZE}}) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_PER_AXIS}} +
  ({{node}}(0) + {{HALO_SIZE}}) * {{NUMBER_OF_VARIABLES_AT_NODE}} +
  {{unknown}};
  {% else %}
  ({{node}}(2) + {{HALO_SIZE}}) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} +
  ({{node}}(1) + {{HALO_SIZE}}) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_PER_AXIS}} +
  ({{node}}(0) + {{HALO_SIZE}}) * {{NUMBER_OF_VARIABLES_AT_NODE}} +
  {{unknown}};
  {% endif %}
{% endmacro %}
