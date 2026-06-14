// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/sycl/Device.h"

#include <sycl/sycl.hpp>

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{STORAGE_PRECISION}}* const                  QInFlattened,
  {{STORAGE_PRECISION}}* const                        QOutFlattened,
  const {{STORAGE_PRECISION}}* const                  QInFaces,
  {{STORAGE_PRECISION}}* const                        QOutFaces,
  const std::uint64_t*                                faceIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
  constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));
  //sycl::queue queue{ sycl::default_selector{} };

{% if COMPUTE_MAX_EIGENVALUE %}
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
{% if COMPUTE_MAX_EIGENVALUE %}
    auto maxRed = sycl::reduction(
      maxEigenvalue,
      sycl::maximum<double>{}
      //, sycl::property::reduction::initialize_to_identity{}
    );
    handle.parallel_for(ndRange, maxRed, [=](sycl::nd_item<{{DIMENSIONS}}> item, auto& red) {
{% else %}
    handle.parallel_for(ndRange, [=](sycl::nd_item<{{DIMENSIONS}}> item) {
{% endif %}
      const auto patchIndex                  = item.get_group(0);
      const {{STORAGE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];
      {{STORAGE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

      const auto patchCentre = tarch::la::Vector<DIMENSIONS, double> {
        patchCentres[patchIndex * {{DIMENSIONS}}],
        patchCentres[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %} , patchCentres[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
      };
      const auto patchSize = tarch::la::Vector<DIMENSIONS, double> {
        patchSizes[patchIndex * {{DIMENSIONS}}],
        patchSizes[patchIndex * {{DIMENSIONS}} + 1]
        {% if DIMENSIONS==3 %} , patchSizes[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
      };

      const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
      {{STORAGE_PRECISION}}* faceQOut[TWO_TIMES_D];
      for (int f = 0; f < TWO_TIMES_D; f++) {
        auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + f];
        faceQIn[f]  = &QInFaces[faceIndex * FaceCardinality];
        faceQOut[f] = &QOutFaces[faceIndex * FaceCardinality];
      }

{% if COMPUTE_MAX_EIGENVALUE %}
      double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

{% if DIMENSIONS==3 %}
      for (int z = item.get_local_id(2); z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
{% endif %}
        for (int y = item.get_local_id(1); y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
          for (int x = item.get_local_id(0); x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
            {{ ComputeFluxesAndPerformTimeStep(False) | indent(10)}}
          } // x
        } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}

{% if COMPUTE_MAX_EIGENVALUE %}
      red.combine(maxEigenvalueAfterTimeStep);
{% endif %}
    });
  });

  event.wait();

{% if COMPUTE_MAX_EIGENVALUE %}
  const double maxEigenvalueAfterTimeStep = *maxEigenvalue;
  sycl::free(maxEigenvalue, queue.get_context());
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
