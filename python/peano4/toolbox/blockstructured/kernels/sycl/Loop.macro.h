// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'CopyPatchAndFillHalo.macro.h' as cp with context %}{# get template macros #}

{% macro loopOverCells() %}
  for (int z = {% if DIMENSIONS==3 %} item.get_local_id(2) {% else %} 0 {% endif %}; z < {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
    for (int y = item.get_local_id(1); y < {{ DOFS_PER_AXIS }}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
      for (int x = item.get_local_id(0); x < {{ DOFS_PER_AXIS }}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
        {{ cp.copyPatchAndFillHalo() | indent(6) }}
      } // x
    } // y
  } // z
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                 = patchData.numberOfCells;
  const {{PRECISION}}* const QInFlattened   = patchData.QInFlattened;
  const {{PRECISION}}* const facesFlattened = patchData.facesFlattened;
  {{PRECISION}}* const QOutFlattened        = patchData.QOutFlattened;

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
      using FaceType          = const {{PRECISION}}(*)[{{FACES_PER_DOF*UNKNOWNS}}];
      const int patchIndex    = item.get_group(0);
      const auto* const QIn   = &QInFlattened[patchIndex  * {{DOFS*UNKNOWNS}}];
      const auto* const faces = reinterpret_cast<FaceType>(&facesFlattened[patchIndex * TwoTimesD * {{FACES_PER_DOF*UNKNOWNS}}]);
      auto* const QOut        = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

      {{ loopOverCells() | indent(2) }}
    });
  });
  event.wait();
{% endmacro %}
