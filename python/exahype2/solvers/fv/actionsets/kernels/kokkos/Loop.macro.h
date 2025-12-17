// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import "Loop.macro.h" as operation with context %}

{% macro loopOverPatches() %}
  auto*                  faces          = patchData.faces;
  const uint64_t*        patchIndices   = patchData.patchIndicesOfBoundaryFaces;
  const uint8_t*         faceNumbers    = patchData.boundaryFaceNumbers;
  const int              numberOfFaces  = patchData.numberOfBoundaryFaces;
  const auto*            cellCentres    = patchData.cellCentre;
  const auto*            cellSizes      = patchData.cellSize;
  const double           t              = patchData.t;

  Kokkos::parallel_for("boundary-face-loop", numberOfFaces, KOKKOS_LAMBDA(const int boundaryFaceIndex) {
    const uint64_t patchIndex = patchIndices[boundaryFaceIndex];
    const int faceNumber      = faceNumbers[boundaryFaceIndex];

    tarch::la::Vector<DIMENSIONS,double> patchSize   = cellSizes[patchIndex];
    tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres[patchIndex];
    double* Q = faces[patchIndex][2][faceNumber];

    {{ operation.inner_loop_macro() | indent(2) }}
  });
{% endmacro %}

{% macro loopOverPatchesNoPatchData() %}
  Kokkos::parallel_for("boundary-face-loop", numberOfFaces, KOKKOS_LAMBDA(const int boundaryFaceIndex) {
    const uint64_t faceIndex = faceIndices[boundaryFaceIndex];
    const int faceNumber     = faceNumbers[boundaryFaceIndex];
    const uint64_t cellIndex = cellIndices[boundaryFaceIndex];

    tarch::la::Vector<DIMENSIONS,double> patchSize   = cellSizes[cellIndex];
    tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres[cellIndex];
    double* Q = &facesFlattened[faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %}];

    {{ operation.inner_loop_macro() | indent(2) }}
  });
{% endmacro %}
