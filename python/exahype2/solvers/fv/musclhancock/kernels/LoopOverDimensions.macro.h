// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverDimensions() %}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) { // Loop over directions
    #if !defined(WITH_GPU)
    assertion(normal >= 0);
    assertion(normal < DIMENSIONS);
    #endif

{% if DIMENSIONS==2 %}
    const auto volumeLeft =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) - {{HALO_SIZE - 1}}, volume(1))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                     volume(1) - {{HALO_SIZE - 1}});
    const auto volumeLeftLeft =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) - {{HALO_SIZE}}, volume(1))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                 volume(1) - {{HALO_SIZE}});
    const auto volumeRight =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) + {{HALO_SIZE - 1}}, volume(1))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                     volume(1) + {{HALO_SIZE - 1}});
    const auto volumeRightRight =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) + {{HALO_SIZE}}, volume(1))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                 volume(1) + {{HALO_SIZE}});
    {% else %}
    const auto volumeLeft =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) - {{HALO_SIZE - 1}}, volume(1),                     volume(2)) :
      normal == 1 ? ::exahype2::fv::toVolumeIndex(volume(0),                     volume(1) - {{HALO_SIZE - 1}}, volume(2))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                     volume(1),                     volume(2) - {{HALO_SIZE - 1}});
    const auto volumeLeftLeft =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) - {{HALO_SIZE}}, volume(1),                 volume(2)) :
      normal == 1 ? ::exahype2::fv::toVolumeIndex(volume(0),                 volume(1) - {{HALO_SIZE}}, volume(2))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                 volume(1),                 volume(2) - {{HALO_SIZE}});
    const auto volumeRight =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) + {{HALO_SIZE - 1}}, volume(1),                     volume(2)) :
      normal == 1 ? ::exahype2::fv::toVolumeIndex(volume(0),                     volume(1) + {{HALO_SIZE - 1}}, volume(2))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                     volume(1),                     volume(2) + {{HALO_SIZE - 1}});
    const auto volumeRightRight =
      normal == 0 ? ::exahype2::fv::toVolumeIndex(volume(0) + {{HALO_SIZE}}, volume(1),                 volume(2)) :
      normal == 1 ? ::exahype2::fv::toVolumeIndex(volume(0),                 volume(1) + {{HALO_SIZE}}, volume(2))
                  : ::exahype2::fv::toVolumeIndex(volume(0),                 volume(1),                 volume(2) + {{HALO_SIZE}});
{% endif %}

    {{ caller() }}
  } // end loop over directions
{% endmacro %}
