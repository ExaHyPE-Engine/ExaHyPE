// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"


namespace exahype2 {
  namespace dg {
    /**
     * Project one quantity from the patch data onto the particle
     *
     * @param marker        Cell marker describing the cell's geometric/spatial
     *   properties.
     * @param voxelsPerAxis Describe patch. We assume that the pathch has
     *   not halo.
     * @param unknownsPerVoxel
     * @param Q             Voxel field, i.e. actual patch data. Has the
     *   dimensions @f$ voxelsPerAxis^d \cdot unknownsPerVoxel @f$.
     * @param particleX     Position of particle.
     * @param unknown       Which unknown from data field to pick.
     */
    template <typename Particle, int SourceIndex, int DestIndex>
    void projectValueOntoParticle(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          order,
      const double* __restrict__                   QuadratureNodes1d,
      int                                          unknownsPerDoF,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    /**
     * Take all values of the unknown field Q and project them onto the
     * particle. In this context, we assume that the tracer's number of
     * entries and the unknowns plus the auxiliary variables have the
     * same total count.
     */
    template <typename Particle, typename QStoreType>
    void projectAllValuesOntoParticle(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          order,
      const double* __restrict__                   QuadratureNodes1d,
      int                                          unknownsPerDoF,
      const QStoreType* __restrict__               Q,
      Particle&                                    particle
    );
  }
}


#include "Tracer.cpph"

