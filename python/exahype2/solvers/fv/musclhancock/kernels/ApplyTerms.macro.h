// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro applySource(input, output, volumeCentre, timeStamp) %}
  {%- if STATELESS_SOURCE_TERM %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm({{input}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, {{output}});
  {%- else -%}
  solver.sourceTerm({{input}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, {{output}});
  {% endif -%}
{% endmacro %}

{% macro applyFlux(input, output, volumeCentre, timeStamp) %}
  {%- if STATELESS_FLUX %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::flux({{input}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, normal, {{output}});
  {%- else -%}
  solver.flux({{input}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, normal, {{output}});
  {% endif -%}
{% endmacro %}

{% macro applyNCP(inputAverage, inputDelta, output, volumeCentre, timeStamp) %}
  {%- if STATELESS_NCP %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct({{inputAverage}}, {{inputDelta}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, normal, {{output}});
  {%- else -%}
  solver.nonconservativeProduct({{inputAverage}}, {{inputDelta}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, normal, {{output}});
  {% endif -%}
{% endmacro %}

{% macro getMaxEigenvalue(input, output, volumeCentre, timeStamp) %}
  {%- if STATELESS_MAX_EIGENVALUE %}
  const {{COMPUTE_PRECISION}} {{output}} = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue({{input}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, normal);
  {%- else -%}
  const {{COMPUTE_PRECISION}} {{output}} = solver.maxEigenvalue({{input}}, {{volumeCentre}}, volumeSize, {{timeStamp}}, timeStepSize, normal);
  {% endif -%}
{% endmacro %}

{% macro applyLimiter(a, b, output) %}
  {% if LIMITER_IMPLEMENTATION=="<none>" %}
  static_assert(false, "No limiter for MUSCL-Hancock is set");
  {% endif -%}

  {%- if STATELESS_LIMITER %}
  {{output}} = {{FULL_QUALIFIED_SOLVER_NAME}}::limit({{a}}, {{b}}) / volumeSize(normal);
  {%- else -%}
  {{output}} = solver.limit({{a}}, {{b}}) / volumeSize(normal);
  {% endif -%}
{% endmacro %}
