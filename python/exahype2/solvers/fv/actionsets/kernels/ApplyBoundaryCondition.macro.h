// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro ApplyBoundaryCondition() %}
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
        volume = ::exahype2::fv::toVolumeIndex(0, x0, x1);
      } else if ((faceNumber%DIMENSIONS) == 1) {
        volume = ::exahype2::fv::toVolumeIndex(x0, 0, x1);
      } else {
        volume = ::exahype2::fv::toVolumeIndex(x1, x0, 0);
      }
      {% else %}
      if ((faceNumber%DIMENSIONS) == 0) {
        volume = ::exahype2::fv::toVolumeIndex(0, x0);
      } else {
        volume = ::exahype2::fv::toVolumeIndex(x0, 0);
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
