// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro index(volume, unknown) %}
  {% if DIMENSIONS==2 %}
  ({{volume}}(1)) * {{PATCH_CARDINALITY_1D}} +
  ({{volume}}(0)) * {{NUMBER_OF_DATA}} +
  {{unknown}};
  {% else %}
  ({{volume}}(2)) * {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} +
  ({{volume}}(1)) * {{PATCH_CARDINALITY_1D}} +
  ({{volume}}(0)) * {{NUMBER_OF_DATA}} +
  {{unknown}};
  {% endif %}
{% endmacro %}
