// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_TRACER_
#define _EXAHYPE2_DG_TRACER_


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


    template <typename Particle>
    void projectAllValuesOntoParticle(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          order,
      const double* __restrict__                   QuadratureNodes1d,
      int                                          unknownsPerDoF,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    namespace internal {
      double projectValueOntoParticle(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          order,
        const double* __restrict__                   QuadratureNodes1d,
        int                                          unknownsPerDoF,
        const double* __restrict__                   Q,
        const tarch::la::Vector<Dimensions,double>&  particleX,
        int                                          unknown
      );
    }
  }
}


#include "Tracer.cpph"

#endif
