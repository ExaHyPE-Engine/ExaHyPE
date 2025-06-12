// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro serialisePatchIndex() %}
  auto serialisePatchIndex = [](const tarch::la::Vector<Dimensions, int>& overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d = 0; d < Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d == normal) {
        base *= {{OVERLAP}} * 2;
      } else {
        base *= {{DOFS_PER_AXIS}};
      }
    }
    return result;
  };
{% endmacro %}
