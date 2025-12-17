// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/sycl/Device.h"

#include <sycl/sycl.hpp>

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&                       solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  [[maybe_unused]] const int                                             targetDevice
) {
  const int numberOfPatches                       = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QInFlattened = patchData.QInFlattened;
  {{COMPUTE_PRECISION}}* const QOutFlattened      = patchData.QOutFlattened;
  const double* const patchCentres                = patchData.cellCentre[0].data();
  const double* const patchSizes                  = patchData.cellSize[0].data();
  const double timeStamp                          = patchData.t;
  const double timeStepSize                       = patchData.dt;

  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));
  //sycl::queue queue{ sycl::default_selector{} };

{% if COMPUTE_MAX_EIGENVALUE==True %}
  double* maxEigenvalue = sycl::malloc_shared<double>(1, queue.get_device(), queue.get_context());
  *maxEigenvalue = 0.0;
{% endif %}

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
{% if COMPUTE_MAX_EIGENVALUE==True %}
    auto maxRed = sycl::reduction(
      maxEigenvalue,
      sycl::maximum<double>{}
      //, sycl::property::reduction::initialize_to_identity{}
    );
    handle.parallel_for(ndRange, maxRed, [=](sycl::nd_item<{{DIMENSIONS}}> item, auto& red) {
{% else %}
    handle.parallel_for(ndRange, [=](sycl::nd_item<{{DIMENSIONS}}> item) {
{% endif %}
      const int patchIndex                   = item.get_group(0);
      const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %}];
      {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

      const tarch::la::Vector<DIMENSIONS, double> patchCentre {
        patchCentres[patchIndex * {{DIMENSIONS}}],
        patchCentres[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %} , patchCentres[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
      };
      const tarch::la::Vector<DIMENSIONS, double> patchSize {
        patchSizes[patchIndex * {{DIMENSIONS}}],
        patchSizes[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %} , patchSizes[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
      };

{% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

{% if DIMENSIONS==3 %}
      for (int z = item.get_local_id(2); z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
{% endif %}
        for (int y = item.get_local_id(1); y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
          for (int x = item.get_local_id(0); x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
            {{ ComputeFluxesAndPerformTimeStep() | indent(10)}}
          } // x
        } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}

{% if COMPUTE_MAX_EIGENVALUE==True %}
      red.combine(maxEigenvalueAfterTimeStep);
{% endif %}
    });
  });

  event.wait();

{% if COMPUTE_MAX_EIGENVALUE==True %}
  const double maxEigenvalueAfterTimeStep = *maxEigenvalue;
  sycl::free(maxEigenvalue, queue.get_context());
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{COMPUTE_PRECISION}}* const                  QInFlattened,
  {{COMPUTE_PRECISION}}* const                        QOutFlattened,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));
  //sycl::queue queue{ sycl::default_selector{} };

{% if COMPUTE_MAX_EIGENVALUE==True %}
  double* maxEigenvalue = sycl::malloc_shared<double>(1, queue.get_device(), queue.get_context());
  *maxEigenvalue = 0.0;
{% endif %}

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
{% if COMPUTE_MAX_EIGENVALUE==True %}
    auto maxRed = sycl::reduction(
      maxEigenvalue,
      sycl::maximum<double>{}
      //, sycl::property::reduction::initialize_to_identity{}
    );
    handle.parallel_for(ndRange, maxRed, [=](sycl::nd_item<{{DIMENSIONS}}> item, auto& red) {
{% else %}
    handle.parallel_for(ndRange, [=](sycl::nd_item<{{DIMENSIONS}}> item) {
{% endif %}
      const int patchIndex                   = item.get_group(0);
      const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %}];
      {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

      const tarch::la::Vector<DIMENSIONS, double> patchCentre {
        patchCentres[patchIndex * {{DIMENSIONS}}],
        patchCentres[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %} , patchCentres[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
      };
      const tarch::la::Vector<DIMENSIONS, double> patchSize {
        patchSizes[patchIndex * {{DIMENSIONS}}],
        patchSizes[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %} , patchSizes[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
      };

{% if COMPUTE_MAX_EIGENVALUE==True %}
      double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

{% if DIMENSIONS==3 %}
      for (int z = item.get_local_id(2); z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
{% endif %}
        for (int y = item.get_local_id(1); y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
          for (int x = item.get_local_id(0); x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
            {{ ComputeFluxesAndPerformTimeStep() | indent(10)}}
          } // x
        } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}

{% if COMPUTE_MAX_EIGENVALUE==True %}
      red.combine(maxEigenvalueAfterTimeStep);
{% endif %}
    });
  });

  event.wait();

{% if COMPUTE_MAX_EIGENVALUE==True %}
  const double maxEigenvalueAfterTimeStep = *maxEigenvalue;
  sycl::free(maxEigenvalue, queue.get_context());
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
