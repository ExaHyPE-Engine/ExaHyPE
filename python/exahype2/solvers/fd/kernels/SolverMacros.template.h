// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'Index.template.h' as id with context %}{# get template macros #}

{% macro createMemoryOnStack() %}
  {{COMPUTE_PRECISION}} QInGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {% if DIMENSIONS==2 %}
  {% for i in range(ORDER) -%}
  {%- if loop.first -%}
  // Closest to node
  {%- elif loop.last %}
  // Furthest from node
  {%- endif -%} {# comments #}
  {{COMPUTE_PRECISION}} QInGatheredLeft{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredRight{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredDown{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredTop{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {% endfor %} {# order range #}
  {% else %}
  {% for i in range(ORDER) -%}
  {%- if loop.first -%}
  // Closest to node
  {%- elif loop.last %}
  // Furthest from node
  {%- endif -%} {# comments #}
  {{COMPUTE_PRECISION}} QInGatheredLeft{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredRight{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredDown{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredTop{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredBack{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {{COMPUTE_PRECISION}} QInGatheredFront{{i}}[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
  {% endfor %} {# order range #}
  {% endif %}

  {{COMPUTE_PRECISION}} QOutGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}] __attribute__((aligned(ALIGNMENT)));
{% endmacro %}

{% macro gatherQ() %}
  // Gather data
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    {% for i in range(ORDER) -%}
    {%- if loop.first -%}
    // Closest to node
    {%- elif loop.last %}
    // Furthest from node
    {%- endif -%} {# comments #}
    const auto indexWithHaloLeft{{i}}   = {{ id.indexWithHalo('nodeLeft'~i, 'unknown') | indent(4) }}{##}
    const auto indexWithHaloRight{{i}}  = {{ id.indexWithHalo('nodeRight'~i, 'unknown') | indent(4) }}{##}
    const auto indexWithHaloDown{{i}}   = {{ id.indexWithHalo('nodeDown'~i, 'unknown') | indent(4) }}{##}
    const auto indexWithHaloTop{{i}}    = {{ id.indexWithHalo('nodeTop'~i, 'unknown') | indent(4) }}{##}

    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHaloLeft{{i}}]));
    assertion(std::isfinite(QIn[indexWithHaloRight{{i}}]));
    assertion(std::isfinite(QIn[indexWithHaloDown{{i}}]));
    assertion(std::isfinite(QIn[indexWithHaloTop{{i}}]));
    #endif

    QInGatheredLeft{{i}}[unknown]   = QIn[indexWithHaloLeft{{i}}];
    QInGatheredRight{{i}}[unknown]  = QIn[indexWithHaloRight{{i}}];
    QInGatheredDown{{i}}[unknown]   = QIn[indexWithHaloDown{{i}}];
    QInGatheredTop{{i}}[unknown]    = QIn[indexWithHaloTop{{i}}];
    {% if DIMENSIONS==3 %}
    const auto indexWithHaloBack{{i}}   = {{ id.indexWithHalo('nodeBack'~i, 'unknown') | indent(4) }}{##}
    const auto indexWithHaloFront{{i}}  = {{ id.indexWithHalo('nodeFront'~i, 'unknown') | indent(4) }}{##}

    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHaloBack{{i}}]));
    assertion(std::isfinite(QIn[indexWithHaloFront{{i}}]));
    #endif

    QInGatheredBack{{i}}[unknown]   = QIn[indexWithHaloBack{{i}}];
    QInGatheredFront{{i}}[unknown]  = QIn[indexWithHaloFront{{i}}];
    {% endif %} {# dimensions #}
    {% endfor %} {# order range #}

    const int indexWithHalo = {{ id.indexWithHalo('node', 'unknown') | indent(4) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QInGathered[unknown] = QIn[indexWithHalo];
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
{% endmacro %}

{% macro scatterQ() %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Scatter data into QOut
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QOutGathered[unknown]));
    #endif
    const int index = {{ id.index('node', 'unknown') | indent(4) }}{##}
    QOut[index] = QOutGathered[unknown];
  } // unknown
{% endmacro %}

{% macro performTimeStep() %}
  {%- if DIMENSIONS == 2 -%}
  {%- set DIMENSION_DESCRIPTOR = ["Left", "Right", "Down", "Top"] -%}
  {%- elif DIMENSIONS == 3 -%}
  {%- set DIMENSION_DESCRIPTOR =  ["Left", "Right", "Down", "Top", "Back", "Front"]  -%}
  {%- endif -%}

  {% if COMPUTE_MAX_EIGENVALUE==True%}
  // Retrieve user data
  {{COMPUTE_PRECISION}} maxEigenvalueGathered = -1 * std::numeric_limits<float>::infinity();
  for (int normal = 0; normal < {{DIMENSIONS}}; ++normal) {
    {% if STATELESS_MAX_EIGENVALUE==True %}
    maxEigenvalueGathered = std::fmax(maxEigenvalueGathered,
      {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInGathered, nodeCentre, h, timeStamp, timeStepSize, normal)
    );
    {% for D in DIMENSION_DESCRIPTOR -%}
    {% for i in range(ORDER) -%}
    maxEigenvalueGathered = std::fmax(maxEigenvalueGathered,
      {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInGathered{{D}}{{i}}, ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_NODES_PER_AXIS}}, node{{D}}{{i}}), h, timeStamp, timeStepSize, normal)
    );
    {%- endfor -%} {# order range #}
    {# Comment for new line #}
    {% endfor %} {# for D #}
    {% else %}
    maxEigenvalueGathered = std::fmax(maxEigenvalueGathered,
      solver.maxEigenvalue(QInGathered, nodeCentre, h, timeStamp, timeStepSize, normal)
    );
    {% for D in DIMENSION_DESCRIPTOR -%}
    {% for i in range(ORDER) -%}
    maxEigenvalueGathered = std::fmax(maxEigenvalueGathered,
      solver.maxEigenvalue(QInGathered{{D}}{{i}}, ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_NODES_PER_AXIS}}, node{{D}}{{i}}), h, timeStamp, timeStepSize, normal)
    );
    {%- endfor -%} {# order range #}
    {# Comment for new line #}
    {% endfor %} {# for D #}
    {% endif %}
  }
  {% endif %} {# if COMPUTE_MAX_EIGENVALUE #}

  {% if STATELESS_SOLVER==True %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::solve(
    QInGathered,
    {# Comment for new line #}
    {% for D in DIMENSION_DESCRIPTOR -%}
    {% for i in range(ORDER) -%}
    {%- if loop.first -%}
    // Closest to node
    {%- elif loop.last -%}
    {# Comment for new line #}
    // Furthest from node
    {%- endif -%} {# comments #}
    QInGathered{{D}}{{i}},
    {%- endfor -%} {# order range #}
    {# Comment for new line #}
    {% endfor %} {# for D #}
    nodeCentre, h,
    timeStamp, timeStepSize,
    QOutGathered
  );
  {% else %}
  solver.solve(
    QInGathered,
    {# Comment for new line #}
    {% for D in DIMENSION_DESCRIPTOR -%}
    {% for i in range(ORDER) -%}
    {%- if loop.first -%}
    // Closest to node
    {%- elif loop.last -%}
    {# Comment for new line #}
    // Furthest from node
    {%- endif -%} {# comments #}
    QInGathered{{D}}{{i}},
    {%- endfor -%} {# order range #}
    {# Comment for new line #}
    {% endfor %} {# for D #}
    nodeCentre, h,
    timeStamp, timeStepSize,
    QOutGathered
  );
  {% endif %}

  {% if COMPUTE_MAX_EIGENVALUE==True%}
  // Compute max eigenvalue (wave speed) after time step
  maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, maxEigenvalueGathered);
  {% endif %}
{% endmacro %}
