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
        {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
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
      {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
      {% endif %}

      {{ riemann.scatterQ() | indent(4) }}
    } // x
  } // y
  {% endif %}
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                                       = patchData.numberOfCells;
  double** const QIns                                             = patchData.QIn;
  double** const QOuts                                            = patchData.QOut;
  const tarch::la::Vector<Dimensions, double>* const patchCentres = patchData.cellCentre;
  const tarch::la::Vector<Dimensions, double>* const patchSizes   = patchData.cellSize;
  const double* const timeStamps                                  = patchData.t;
  const double* const timeStepSizes                               = patchData.dt;

  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const double* const QIn                                       = QIns[patchIndex];
    double* const QOut                                            = QOuts[patchIndex];
    const tarch::la::Vector<Dimensions, double>& patchCentre      = patchCentres[patchIndex];
    const tarch::la::Vector<Dimensions, double>& patchSize        = patchSizes[patchIndex];
    const double timeStamp                                        = timeStamps[patchIndex];
    const double timeStepSize                                     = timeStepSizes[patchIndex];

    {{ loopOverVolumes() | indent(2) }}
  } // patchIndex
{% endmacro %}
