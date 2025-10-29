// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro inner_loop_macro() %}
  auto serialisePatchIndex = [](const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= {{HALO_SIZE}} * 2;
      } else {
        base *= {{NUMBER_OF_VOLUMES_PER_AXIS}};
      }
    }
    return result;
  };

  tarch::la::Vector<DIMENSIONS,double> volumeH    = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
  tarch::la::Vector<DIMENSIONS,double> faceOffset = patchCentre - 0.5 * patchSize;

  if (faceNumber >= DIMENSIONS) {
    faceOffset(faceNumber%DIMENSIONS) += patchSize(faceNumber%DIMENSIONS);
  }

  for (int x1 = 0; x1 < {% if DIMENSIONS==3 %} {{NUMBER_OF_VOLUMES_PER_AXIS}} {% else %} 1 {% endif %}; x1++) {
    for (int x0 = 0; x0 < {{NUMBER_OF_VOLUMES_PER_AXIS}}; x0++) {
      tarch::la::Vector<DIMENSIONS, int> volume;
      {% if DIMENSIONS==3 %}
      if ((faceNumber%DIMENSIONS) == 0) {
        volume = {0, x0, x1};
      } else if ((faceNumber%DIMENSIONS) == 1) {
        volume = {x0, 0, x1};
      } else {
        volume = {x1, x0, 0};
      }
      {% else %}
      if ((faceNumber%DIMENSIONS) == 0) {
        volume = {0, x0};
      } else {
        volume = {x0, 0};
      }
      {% endif %}

      tarch::la::Vector<DIMENSIONS,int>    insideVolume  = volume;
      tarch::la::Vector<DIMENSIONS,int>    outsideVolume = volume;
      tarch::la::Vector<DIMENSIONS,double> xVector       = faceOffset + tarch::la::multiplyComponents( tarch::la::convertScalar<double>(volume)+tarch::la::Vector<DIMENSIONS,double>(0.5), volumeH);

      xVector(faceNumber%DIMENSIONS) -= 0.5 * volumeH(faceNumber%DIMENSIONS);

      for (int layer=0; layer<{{HALO_SIZE}}; layer++){
        const int dir = faceNumber % DIMENSIONS;

        insideVolume(dir)  = (faceNumber < DIMENSIONS) ?
          ({{HALO_SIZE}} - layer) : (layer + {{HALO_SIZE}} - 1);

        outsideVolume(dir) = (faceNumber < DIMENSIONS) ?
          ({{HALO_SIZE}} - 1 - layer) : (layer + {{HALO_SIZE}});

        const int insideVolumeSerialised = serialisePatchIndex(insideVolume, dir);
        int outsideVolumeSerialised = serialisePatchIndex(outsideVolume, dir);

        const int stride = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
        {% if STATELESS_BOUNDARY_CONDITIONS==True %}
        {{FULL_QUALIFIED_SOLVER_NAME}}::boundaryConditions(
          Q + insideVolumeSerialised  * stride,
          Q + outsideVolumeSerialised * stride,
          xVector, volumeH, t, faceNumber
        );
        {% else %}
        solver.boundaryConditions(
          Q + insideVolumeSerialised  * stride,
          Q + outsideVolumeSerialised * stride,
          xVector, volumeH, t, faceNumber
        );
        {% endif %}
      }
    } // x0
  } // x1
{% endmacro %}

{% macro loopOverPatches() %}
  {{STORAGE_PRECISION}}* facesFlattened = patchData.facesFlattened;
  const uint64_t*        faceIndices    = patchData.boundaryFaceIndices;
  const uint8_t*         faceNumbers    = patchData.boundaryFaceNumbers;
  const int              numberOfFaces  = patchData.numberOfBoundaryFaces;
  const auto*            cellCentres    = patchData.cellCentre;
  const auto*            cellSizes      = patchData.cellSize;
  const double           t              = patchData.t;

  {% if MULTITHREADING == "ON" %}
  #pragma omp parallel for schedule(static)
  {% endif %}
  for (int boundaryFaceIndex = 0; boundaryFaceIndex < numberOfFaces; ++boundaryFaceIndex) {
    const uint64_t faceIndex  = faceIndices[boundaryFaceIndex];
    const int      faceNumber = faceNumbers[boundaryFaceIndex];

    tarch::la::Vector<DIMENSIONS,double> patchSize = cellSizes[faceIndex];
    tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres[faceIndex];
    double* Q = &facesFlattened[faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %}];

    {{ inner_loop_macro() | indent(2) }}
  }
{% endmacro %}
