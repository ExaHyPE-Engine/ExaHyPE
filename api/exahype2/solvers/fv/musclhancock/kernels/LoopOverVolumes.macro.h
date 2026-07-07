// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverVolumes() %}
  // Loop over volumes
  {% if DIMENSIONS == 3 %}
  for (int z = 0; z < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++z) {
    for (int y = 0; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++y) {
      for (int x = 0; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++x) {
        const auto volume = ::exahype2::fv::toVolumeIndex(x, y, z);
        const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
        const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);
        {{ caller() }}
      } // x
    } // y
  } // z
  {% else %}
  for (int y = 0; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++y) {
    for (int x = 0; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; ++x) {
      const auto volume = ::exahype2::fv::toVolumeIndex(x, y);
      const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
      const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);
      {{ caller() }}
    } // x
  } // y
  {% endif %}
{% endmacro %}
