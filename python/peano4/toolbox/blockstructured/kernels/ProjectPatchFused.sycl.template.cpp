// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchFused.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Projection.h"

{% from 'CopyPatchOntoFaces.macro.h' import CopyPatchOntoFaces with context %}

#include "tarch/accelerator/sycl/Device.h"
#include <sycl/sycl.hpp>

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  projectPatchFused(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.faceIndices,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatchFused(
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}*       const NOALIAS facePool,
  const std::uint64_t*               faceIndices,
  const int                          numberOfPatches,
  const int                          targetDevice
) {
  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));

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
      const int patchIndex  = item.get_group(0);
      const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];

{% if DIMENSIONS==3 %}
      for (int z = item.get_local_id(2); z < {{DOFS_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
{% endif %}
        for (int y = item.get_local_id(1); y < {{DOFS_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
          for (int x = item.get_local_id(0); x < {{DOFS_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
            {{ CopyPatchOntoFaces() | indent(6) }}
          } // x
        } // y
{% if DIMENSIONS==3 %}
      } // z
{% endif %}
    });
  });
  event.wait();
}
