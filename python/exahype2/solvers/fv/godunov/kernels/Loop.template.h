// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

{% macro loopOverDimensions() %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) { // Loop over directions
    #ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    #endif

    {% if DIMENSIONS==2 %}
    const auto volumeLeft =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) - {{HALO_SIZE}}, volume(1))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1) - {{HALO_SIZE}});
    const auto volumeRight =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) + {{HALO_SIZE}}, volume(1))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1) + {{HALO_SIZE}});
    {% else %}
    const auto volumeLeft =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) - {{HALO_SIZE}}, volume(1),                 volume(2)) :
      normal == 1 ? ::exahype2::volumeIndex(volume(0),                 volume(1) - {{HALO_SIZE}}, volume(2))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1),                 volume(2) - {{HALO_SIZE}});
    const auto volumeRight =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) + {{HALO_SIZE}}, volume(1),                 volume(2)) :
      normal == 1 ? ::exahype2::volumeIndex(volume(0),                 volume(1) + {{HALO_SIZE}}, volume(2))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1),                 volume(2) + {{HALO_SIZE}});
    {% endif %}

    {{ riemann.computeFluxesAndPerformTimeStep() | indent(2) }}
  } // end loop over directions
{% endmacro %}

{% macro loopOverVolumes() %}
  // Loop over volumes
  {% if DIMENSIONS == 3 %}
  for (int z = 0; z < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++z) {
    for (int y = 0; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++y) {
      for (int x = 0; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++x) {
        const auto volume = ::exahype2::volumeIndex(x, y, z);
        const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
        const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);

        {{ riemann.createMemoryOnStack() | indent(4) }}
        {{ riemann.gatherQ() | indent(4) }}

        {{ loopOverDimensions() | indent(4) }}

        {% if COMPUTE_MAX_EIGENVALUE==True %}
        {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }}
        {% endif %}

        {{ riemann.scatterQ() | indent(4) }}
      } // x
    } // y
  } // z
  {% else %}
  for (int y = 0; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++y) {
    for (int x = 0; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++x) {
      const auto volume = ::exahype2::volumeIndex(x, y);
      const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
      const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);

      {{ riemann.createMemoryOnStack() | indent(4) }}
      {{ riemann.gatherQ() | indent(4) }}

      {{ loopOverDimensions() | indent(4) }}

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }}
      {% endif %}

      {{ riemann.scatterQ() | indent(4) }}
    } // x
  } // y
  {% endif %}
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches               = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QIns = patchData.QIn1D;
  {{COMPUTE_PRECISION}}* const QOuts      = patchData.QOut1D;
  const double* const patchCentres        = patchData.cellCentre[0].data();
  const double* const patchSizes          = patchData.cellSize[0].data();
  const double timeStamp                  = patchData.t;
  const double timeStepSize               = patchData.dt;

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  {% endif %}

  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const {{COMPUTE_PRECISION}}* const QIn = &QIns[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}];
    {{COMPUTE_PRECISION}}* const QOut      = &QOuts[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];

    const tarch::la::Vector<Dimensions, double> patchCentre {
      patchCentres[patchIndex * {{DIMENSIONS}}],
      patchCentres[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %}
      ,
      patchCentres[patchIndex * {{DIMENSIONS}} + 2]
      {% endif %}
    };
    const tarch::la::Vector<Dimensions, double> patchSize {
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
