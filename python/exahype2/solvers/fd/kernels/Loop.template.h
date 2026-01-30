// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'SolverMacros.template.h' as solverMacros with context %}{# get template macros #}

{% macro loopOverDIMENSIONS() %}
  {% if DIMENSIONS==2 %}
  {% for i in range(ORDER) -%}
  {%- if loop.first -%}
  // Closest to node
  {%- elif loop.last %}
  // Furthest from node
  {%- endif -%} {# comments #}
  const auto nodeLeft{{i}}  = ::exahype2::fv::toVolumeIndex(node(0) - {{i + 1}}, node(1));
  const auto nodeRight{{i}} = ::exahype2::fv::toVolumeIndex(node(0) + {{i + 1}}, node(1));
  const auto nodeDown{{i}}  = ::exahype2::fv::toVolumeIndex(node(0),             node(1) - {{i + 1}});
  const auto nodeTop{{i}}   = ::exahype2::fv::toVolumeIndex(node(0),             node(1) + {{i + 1}});
  {% endfor %} {# order range #}
  {% else %} {# dimensions #}
  {% for i in range(ORDER) -%}
  {%- if loop.first -%}
  // Closest to node
  {%- elif loop.last %}
  // Furthest from node
  {%- endif -%} {# comments #}
  const auto nodeLeft{{i}}  = ::exahype2::fv::toVolumeIndex(node(0) - {{i + 1}}, node(1),             node(2));
  const auto nodeRight{{i}} = ::exahype2::fv::toVolumeIndex(node(0) + {{i + 1}}, node(1),             node(2));
  const auto nodeDown{{i}}  = ::exahype2::fv::toVolumeIndex(node(0),             node(1) - {{i + 1}}, node(2));
  const auto nodeTop{{i}}   = ::exahype2::fv::toVolumeIndex(node(0),             node(1) + {{i + 1}}, node(2));
  const auto nodeBack{{i}}  = ::exahype2::fv::toVolumeIndex(node(0),             node(1),             node(2) - {{i + 1}});
  const auto nodeFront{{i}} = ::exahype2::fv::toVolumeIndex(node(0),             node(1),             node(2) + {{i + 1}});
  {% endfor %} {# order range #}
  {% endif %} {# dimensions #}

  {{ solverMacros.gatherQ() | indent(2) }}
  {{ solverMacros.performTimeStep() | indent(2) }}
{% endmacro %}

{% macro loopOverVolumes() %}
  // Loop over volumes
  {% if DIMENSIONS == 3 %}
  for (int z = 0; z < {{ NUMBER_OF_NODES_PER_AXIS }}; ++z) {
    for (int y = 0; y < {{ NUMBER_OF_NODES_PER_AXIS }}; ++y) {
      for (int x = 0; x < {{ NUMBER_OF_NODES_PER_AXIS }}; ++x) {
        const auto node       = ::exahype2::fv::toVolumeIndex(x, y, z);
        const auto h          = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_NODES_PER_AXIS}});
        const auto nodeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_NODES_PER_AXIS}}, node);
        {{ solverMacros.createMemoryOnStack() | indent(4) }}
        {{ loopOverDIMENSIONS() | indent(4) }}
        {{ solverMacros.scatterQ() | indent(4) }}
      } // x
    } // y
  } // z
  {% else %}
  for (int y = 0; y < {{ NUMBER_OF_NODES_PER_AXIS }}; ++y) {
    for (int x = 0; x < {{ NUMBER_OF_NODES_PER_AXIS }}; ++x) {
      const auto node       = ::exahype2::fv::toVolumeIndex(x, y);
      const auto h          = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_NODES_PER_AXIS}});
      const auto nodeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_NODES_PER_AXIS}}, node);
      {{ solverMacros.createMemoryOnStack() | indent(4) }}
      {{ loopOverDIMENSIONS() | indent(4) }}
      {{ solverMacros.scatterQ() | indent(4) }}
    } // x
  } // y
  {% endif %}
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                       = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QInFlattened = patchData.QInFlattened;
  {{COMPUTE_PRECISION}}* const QOutFlattened      = patchData.QOutFlattened;
  const double* const patchCentres                = patchData.cellCentre[0].data();
  const double* const patchSizes                  = patchData.cellSize[0].data();
  const double timeStamp                          = patchData.t;
  const double timeStepSize                       = patchData.dt;

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  {% endif %}

  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}];
    {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];

    const tarch::la::Vector<DIMENSIONS, double> patchCentre {
      patchCentres[patchIndex * {{DIMENSIONS}}],
      patchCentres[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %}
      ,
      patchCentres[patchIndex * {{DIMENSIONS}} + 2]
      {% endif %}
    };
    const tarch::la::Vector<DIMENSIONS, double> patchSize {
      patchSizes[patchIndex * {{DIMENSIONS}}],
      patchSizes[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %}
      ,
      patchSizes[patchIndex * {{DIMENSIONS}} + 2]
      {% endif %}
    };

    {{ loopOverVolumes() | indent(2) }}
  } // patchIndex

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
{% endmacro %}
