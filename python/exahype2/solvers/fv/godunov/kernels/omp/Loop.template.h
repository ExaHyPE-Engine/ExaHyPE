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
  #pragma omp parallel for collapse(3) {% if COMPUTE_MAX_EIGENVALUE==True %} reduction(max : maxEigenvalueAfterTimeStep) {% endif %}
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
        {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }}
        {% endif %}

        {{ riemann.scatterQ() | indent(4) }}
      } // x
    } // y
  } // z
  {% else %}
  {% if GPU == "omp" %}
  #pragma omp parallel for collapse(2) {% if COMPUTE_MAX_EIGENVALUE==True %} reduction(max : maxEigenvalueAfterTimeStep) {% endif %}
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
      {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }}
      {% endif %}

      {{ riemann.scatterQ() | indent(4) }}
    } // x
  } // y
  {% endif %}
{% endmacro %}

{% macro loopOverPatches() %}
  for (int task = 0; task < {{NUMBER_OF_ENCLAVE_TASKS}}; task++) {
    const int startPatch = task * (patchData.numberOfCells / {{NUMBER_OF_ENCLAVE_TASKS}}) + std::min(task, patchData.numberOfCells % {{NUMBER_OF_ENCLAVE_TASKS}});
    const int endPatch   = (task + 1) * (patchData.numberOfCells / {{NUMBER_OF_ENCLAVE_TASKS}}) + std::min(task + 1, patchData.numberOfCells % {{NUMBER_OF_ENCLAVE_TASKS}});
    const int numberOfPatches = endPatch - startPatch;

    const {{COMPUTE_PRECISION}}* const QInFlattened     = patchData.QInFlattened + startPatch * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %};
    {{COMPUTE_PRECISION}}* const QOutFlattened          = patchData.QOutFlattened + startPatch * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %};
    const double* const patchCentres                    = patchData.cellCentre[0].data() + startPatch * {{DIMENSIONS}};
    const double* const patchSizes                      = patchData.cellSize[0].data() + startPatch * {{DIMENSIONS}};
    {% if COMPUTE_MAX_EIGENVALUE==True %}
    {{COMPUTE_PRECISION}}* const maxEigenvalues         = patchData.maxEigenvalue + startPatch;
    {% endif %}
    const double timeStamp                              = patchData.t;
    const double timeStepSize                           = patchData.dt;

    {% if GPU == "omp" %}
    #ifdef WITH_TARGETDART // Use nowait to spawn tasks asynchronously
    {% if COMPUTE_MAX_EIGENVALUE==True %}
    #pragma omp target teams distribute device(targetDevice) nowait \
    map(to: \
      QInFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}], \
      patchCentres[0: numberOfPatches * {{DIMENSIONS}}], \
      patchSizes[0: numberOfPatches * {{DIMENSIONS}}] \
    ) \
    map(from: \
      QOutFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}], \
      maxEigenvalues[0: numberOfPatches] \
    )
    {% else %}
    #pragma omp target teams distribute device(targetDevice) nowait \
    map(to: \
      QInFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}], \
      patchCentres[0: numberOfPatches * {{DIMENSIONS}}], \
      patchSizes[0: numberOfPatches * {{DIMENSIONS}}] \
    ) \
    map(from: \
      QOutFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}] \
    )
    {% endif %}
    #else
    {% if COMPUTE_MAX_EIGENVALUE==True %}
    #pragma omp target teams distribute device(targetDevice) \
    map(to: \
      QInFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}], \
      patchCentres[0: numberOfPatches * {{DIMENSIONS}}], \
      patchSizes[0: numberOfPatches * {{DIMENSIONS}}] \
    ) \
    map(from: \
      QOutFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}], \
      maxEigenvalues[0: numberOfPatches] \
    )
    {% else %}
    #pragma omp target teams distribute device(targetDevice) \
    map(to: \
      QInFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}], \
      patchCentres[0: numberOfPatches * {{DIMENSIONS}}], \
      patchSizes[0: numberOfPatches * {{DIMENSIONS}}] \
    ) \
    map(from: \
      QOutFlattened[0: numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}] \
    )
    {% endif %}
    #endif
    {% else %}
    #pragma omp parallel for
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

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueAfterTimeStep = 0.0;
      {% endif %}

      {{ loopOverVolumes() | indent(4) }}

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      maxEigenvalues[patchIndex] = maxEigenvalueAfterTimeStep;
      {% endif %}
    } // patchIndex
  } // task

  #ifdef WITH_TARGETDART
  #pragma omp taskwait
  #endif

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  const int numberOfPatches = patchData.numberOfCells;
  {{COMPUTE_PRECISION}} const* maxEigenvalues = patchData.maxEigenvalue;
  // #pragma omp target device(targetDevice) map(to: maxEigenvalues[numberOfPatches])
  // #pragma omp parallel for reduction(max: maxEigenvalueAfterTimeStep)
  for (int i = 0; i < numberOfPatches; ++i) {
    maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, maxEigenvalues[i]);
  }
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
{% endmacro %}
