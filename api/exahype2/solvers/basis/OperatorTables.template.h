// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

/**
{{ TABLE_CLASS_DOCUMENTATION }} *
 * initialise() allocates the arrays and fills them, deallocate() frees them. The solver's
 * startSimulation() calls initialise() before the first grid traversal.
 */
template <typename Precision>
class {{ TABLE_CLASS }} final {
public:
  static void initialise();
  static void deallocate();
{% for table in TABLES %}
  /**
{{ table.documentation }}   */
  inline static Precision* {{ table.name }} = nullptr;
{% endfor %}};

{% for item in NAMESPACE -%}
}
{%- endfor %}
