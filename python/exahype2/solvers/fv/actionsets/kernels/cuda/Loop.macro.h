// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import "Loop.macro.h" as operation with context %}

{% macro defineGlobalKernel() %}
__global__ static void
{% if DIMENSIONS==3 %} __launch_bounds__(32) {% else %} __launch_bounds__(32) {% endif %}
kernel(
  {{STORAGE_PRECISION}}* const                               (*faces)[3][TWO_TIMES_D],
  const uint64_t*              __restrict__                  patchIndices,
  const uint8_t*               __restrict__                  faceNumbers,
  const tarch::la::Vector<DIMENSIONS, double>* __restrict__  cellCentres,
  const tarch::la::Vector<DIMENSIONS, double>* __restrict__  cellSizes,
  const double                                               t,
  const int                                                  numberOfFaces
) {
  if (blockIdx.x >= (unsigned)numberOfFaces) return;
  if (threadIdx.x | threadIdx.y | threadIdx.z) return;

  const auto boundaryFaceIndex = blockIdx.x;
  const uint64_t    patchIndex = patchIndices[boundaryFaceIndex];
  const int         faceNumber = faceNumbers[boundaryFaceIndex];

  tarch::la::Vector<DIMENSIONS,double> patchSize   = cellSizes [patchIndex];
  tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres[patchIndex];

  double* Q = faces[patchIndex][2][faceNumber];

  {{ operation.inner_loop_macro() | indent(2) }}
}

__global__ static void
{% if DIMENSIONS==3 %} __launch_bounds__(32) {% else %} __launch_bounds__(32) {% endif %}
kernel_no_patch_data(
  {{STORAGE_PRECISION}}* const                               facesFlattened,
  const uint64_t*              __restrict__                  faceIndices,
  const uint8_t*               __restrict__                  faceNumbers,
  const tarch::la::Vector<DIMENSIONS, double>* __restrict__  cellCentres,
  const tarch::la::Vector<DIMENSIONS, double>* __restrict__  cellSizes,
  const uint64_t*              __restrict__                  cellIndices,
  const double                                               t,
  const int                                                  numberOfFaces
) {
  if (blockIdx.x >= (unsigned)numberOfFaces) return;
  if (threadIdx.x | threadIdx.y | threadIdx.z) return;

  const auto boundaryFaceIndex = blockIdx.x;
  const uint64_t    faceIndex = faceIndices[boundaryFaceIndex];
  const int        faceNumber = faceNumbers[boundaryFaceIndex];
  const uint64_t    cellIndex = cellIndices[boundaryFaceIndex];

  tarch::la::Vector<DIMENSIONS,double> patchSize   = cellSizes[cellIndex];
  tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres[cellIndex];

  double* Q = &facesFlattened[faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %}];

  {{ operation.inner_loop_macro() | indent(2) }}
}
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfFaces = patchData.numberOfBoundaryFaces;
  if (numberOfFaces <= 0) {
    return;
  }

  auto*                  faces          = patchData.faces;
  const uint64_t*        patchIndices   = patchData.patchIndicesOfBoundaryFaces;
  const uint8_t*         faceNumbers    = patchData.boundaryFaceNumbers;
  const auto*            cellCentres    = patchData.cellCentre;
  const auto*            cellSizes      = patchData.cellSize;
  const double           t              = patchData.t;

  const dim3 dimBlock(1, 1, 1);
  const dim3 dimGrid(numberOfFaces, 1, 1);

  CHECK_CUDA_ERROR(cudaSetDevice(targetDevice));
  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    faces, patchIndices, faceNumbers, cellCentres, cellSizes, t, numberOfFaces
  );
  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::deviceStreamGet()));
{% endmacro %}

{% macro loopOverPatchesNoPatchData() %}
  if (numberOfFaces <= 0) {
    return;
  }

  const dim3 dimBlock(1, 1, 1);
  const dim3 dimGrid(numberOfFaces, 1, 1);

  CHECK_CUDA_ERROR(cudaSetDevice(targetDevice));
  kernel_no_patch_data<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    facesFlattened, faceIndices, faceNumbers, cellCentres, cellSizes, cellIndices, t, numberOfFaces
  );
  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::deviceStreamGet()));
{% endmacro %}
