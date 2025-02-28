// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

{% import 'Loop.template.h' as loop with context %}{# get template macros #}

{% macro loopOverVolumes() %}
  // Loop over volumes
  {% if DIMENSIONS == 3 %}
  {% if GPU == "omp" %}
  #pragma omp parallel for simd collapse(3) reduction(max : maxEigenvalueAfterTimeStep)
  {% else %}
  #pragma omp simd collapse(3)
  {% endif %}
  for (int z = 0; z < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++z) {
    for (int y = 0; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++y) {
      for (int x = 0; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++x) {
        const auto volume = ::exahype2::volumeIndex(x, y, z);
        const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
        const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);

        {{ riemann.createMemoryOnStack() | indent(4) }}
        {{ riemann.gatherQ() | indent(4) }}

        {{ loop.loopOverDimensions() | indent(4) }}

        {% if COMPUTE_MAX_EIGENVALUE==True %}
        {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
        {% endif %}

        {{ riemann.scatterQ() | indent(4) }}
      } // x
    } // y
  } // z
  {% else %}
  {% if GPU == "omp" %}
  #pragma omp parallel for simd collapse(2) reduction(max : maxEigenvalueAfterTimeStep)
  {% else %}
  #pragma omp simd collapse(2)
  {% endif %}
  for (int y = 0; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++y) {
    for (int x = 0; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++x) {
      const auto volume = ::exahype2::volumeIndex(x, y);
      const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
      const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);

      {{ riemann.createMemoryOnStack() | indent(4) }}
      {{ riemann.gatherQ() | indent(4) }}

      {{ loop.loopOverDimensions() | indent(4) }}

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
      {% endif %}

      {{ riemann.scatterQ() | indent(4) }}
    } // x
  } // y
  {% endif %}
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches         = patchData.numberOfCells;
  const double* const patchCentres  = patchData.cellCentre[0].data();
  const double* const patchSizes    = patchData.cellSize[0].data();
  const double* const timeStamps    = patchData.t;
  const double* const timeStepSizes = patchData.dt;

  {% if GPU == "omp" %}
  // TODO: We need to get rid of these memory allocations and the memory copies below.
  double* QIns = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
    tarch::MemoryLocation::Heap,
    targetDevice
  );

  double* QOuts = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
    tarch::MemoryLocation::Heap,
    targetDevice
  );

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::copy_n(
      patchData.QIn[patchIndex],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
      &QIns[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}]
    );
  }
  {% else %}
  double** const QIns = patchData.QIn;
  double** const QOuts = patchData.QOut;
  {% endif %}

  {% if GPU == "omp" %}
  #pragma omp target teams distribute device(targetDevice) reduction(max : maxEigenvalueAfterTimeStep) nowait depend(out: patchData.QOut) \
    map(to: \
      numberOfPatches, \
      QIns[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}], \
      patchCentres[0: numberOfPatches * {{DIMENSIONS}}], \
      patchSizes[0: numberOfPatches * {{DIMENSIONS}}], \
      timeStamps[0: numberOfPatches], \
      timeStepSizes[0: numberOfPatches]) \
    map(from: \
      QOuts[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}])
  {% else %}
  #pragma omp parallel for {% if COMPUTE_MAX_EIGENVALUE==True %} reduction(max : maxEigenvalueAfterTimeStep) {% endif %}
  {% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    {% if GPU == "omp" %}
    const double* const QIn = &QIns[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}];
    double* const QOut      = &QOuts[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];
    {% else %}
    const double* const QIn = QIns[patchIndex];
    double* const QOut      = QOuts[patchIndex];
    {% endif %}

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

    const double timeStamp    = timeStamps[patchIndex];
    const double timeStepSize = timeStepSizes[patchIndex];

    {{ loopOverVolumes() }}
  } // patchIndex

  {% if GPU == "omp" %}
  #pragma omp taskwait depend(in: patchData.QOut)
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::copy_n(
      &QOuts[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
      patchData.QOut[patchIndex]
    );
  }

  tarch::accelerator::MemoryManager::getInstance().free(QIns, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(QOuts, targetDevice);
  {% endif %}
{% endmacro %}
