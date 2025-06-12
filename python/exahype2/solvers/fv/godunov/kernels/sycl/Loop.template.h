// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

{% import 'Loop.template.h' as loop with context %}{# get template macros #}

{% macro loopOverVolumes() %}
  {% if DIMENSIONS == 3 %}
  for (int z = item.get_local_id(2); z < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; z += 8) {
    for (int y = item.get_local_id(1); y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; y += 8) {
      for (int x = item.get_local_id(0); x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; x += 8) {
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
  for (int y = item.get_local_id(1); y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; y += 16) {
    for (int x = item.get_local_id(0); x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; x += 16) {
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
  const int numberOfPatches                       = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QInFlattened = patchData.QInFlattened;
  {{COMPUTE_PRECISION}}* const QOutFlattened      = patchData.QOutFlattened;
  const double* const patchCentres                = patchData.cellCentre[0].data();
  const double* const patchSizes                  = patchData.cellSize[0].data();
  const double timeStamp                          = patchData.t;
  const double timeStepSize                       = patchData.dt;

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  double* maxEigenvalue = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    1,
    tarch::MemoryLocation::Pinned,
    targetDevice
  );
  // Initialise to zero
  tarch::copyTo(maxEigenvalue, &maxEigenvalueAfterTimeStep, 1, tarch::MemoryLocation::Pinned, targetDevice);
  {% endif %}

  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));
  //sycl::queue queue{ sycl::default_selector{} };

  {% if DIMENSIONS == 3 %}
  auto ndRange = sycl::nd_range(
    sycl::range(numberOfPatches * 8, 8, 8),
    sycl::range(8, 8, 8)
  );
  {% else %}
  auto ndRange = sycl::nd_range(
    sycl::range(numberOfPatches * 16, 16),
    sycl::range(16, 16)
  );
  {% endif %}

  sycl::event event = queue.submit([&](sycl::handler& handle) {
    handle.parallel_for(ndRange, [=](sycl::nd_item<{{DIMENSIONS}}> item) {
      const int patchIndex    = item.get_group(0);
      const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}];
      {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];

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

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueAfterTimeStep = 0.0;
      {% endif %}

      {{ loopOverVolumes() | indent(4) }}

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueOverGroup = sycl::reduce_over_group(item.get_group(), maxEigenvalueAfterTimeStep, sycl::maximum<>());
      if (item.get_local_id(0) == 0 and item.get_local_id(1) == 0 {% if DIMENSIONS==3 %} and item.get_local_id(2) == 0 {% endif %}) {
        sycl::atomic_ref<double, sycl::memory_order::relaxed, sycl::memory_scope::device, sycl::access::address_space::global_space> atomicGlobalMax(maxEigenvalue[0]);
        // TODO: Not supported by Intel oneAPI
        atomicGlobalMax.fetch_max(maxEigenvalueOverGroup);
      }
      {% endif %}
    });
  });
  event.wait();

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  tarch::copyFrom(
    maxEigenvalue,
    &maxEigenvalueAfterTimeStep,
    1,
    tarch::MemoryLocation::Pinned,
    targetDevice
  );
  tarch::accelerator::MemoryManager::getInstance().free(maxEigenvalue, targetDevice);
  {% endif %}

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
{% endmacro %}
