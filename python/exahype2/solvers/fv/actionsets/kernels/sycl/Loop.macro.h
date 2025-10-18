// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import "Loop.macro.h" as operation with context %}

{% macro loopOverPatches() %}
  {{STORAGE_PRECISION}}* facesFlattened = patchData.facesFlattened;
  const uint64_t*        faceIndices    = patchData.boundaryFaceIndices;
  const uint8_t*         faceNumbers    = patchData.boundaryFaceNumbers;
  const int              numberOfFaces  = patchData.numberOfBoundaryFaces;
  const auto*            cellCentres    = patchData.cellCentre;
  const auto*            cellSizes      = patchData.cellSize;
  const double           t              = patchData.t;

  if (numberOfFaces <= 0) {
    return;
  }

  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));

  auto facesFlattened_dev = facesFlattened;
  auto faceIndices_dev    = faceIndices;
  auto faceNumbers_dev    = faceNumbers;
  auto cellCentres_dev    = cellCentres;
  auto cellSizes_dev      = cellSizes;
  const double t_dev      = t;

  size_t nFaces = numberOfFaces;

  sycl::event event = queue.submit([&](sycl::handler& h) {
    h.parallel_for(sycl::range<1>(nFaces), [=](sycl::id<1> gid) {
      auto boundaryFaceIndex = gid[0];

      uint64_t faceIndex  = faceIndices_dev[boundaryFaceIndex];
      int      faceNumber = faceNumbers_dev[boundaryFaceIndex];

      tarch::la::Vector<DIMENSIONS,double> patchSize   = cellSizes_dev[faceIndex];
      tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres_dev[faceIndex];

      double* Q = &facesFlattened_dev[
        faceIndex * ({% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %})
      ];

      {{ operation.inner_loop_macro() | indent(6) }}
    });
  });

  event.wait();
{% endmacro %}
