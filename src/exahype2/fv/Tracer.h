// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"


namespace exahype2 {
  namespace fv {
    /**
     * Accessor for a tracer particle
     *
     * The toolbox particle routines require an accessor when you use their
     * time integration (and other routines). This is the specific accessor for
     * a tracer in the Finite Volume context.
     *
     * @see toolbox::particles::ParticleAccessor
     */
    template <typename Particle>
    struct MapValuesOntoParticleVelocitiesAccessor_PiecewiseConstant {
      tarch::la::Vector< Dimensions, double>& x(Particle& particle) const {
        return particle.x();
      }

      const tarch::la::Vector< Dimensions, double>& x(const Particle& particle) const {
        return particle.x();
      }

      tarch::la::Vector< Dimensions, double>& v(Particle& particle) const {
        return particle.v();
      }

      const tarch::la::Vector< Dimensions, double>& v(const Particle& particle) const {
        return particle.v();
      }
    };


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
    void projectValueOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    template <typename Particle>
    void projectAllValuesOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    template <typename Particle, int SourceIndex, int DestIndex>
    void projectValueOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    template <typename Particle>
    void projectAllValuesOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    namespace internal {
      /**
       * Assume that we have a particle suspended in a cell. The cell hosts a
       * regular Cartesian mesh. The routine computes the correct voxel.
       *
       * You can convert the result via
       *
       *       int voxelIndex = peano4::utils::dLinearised(voxel,voxelsPerAxis);
       *
       * into an index to access your Q array. The whole construct assumes that
       * Q is not surrounded by a halo layer. If you have a halo layer, you have
       * to add (1,1,1) to the result and then increment voxelsPerAxis by two.
       */
      tarch::la::Vector<Dimensions,int> mapParticleOntoVoxel(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        const tarch::la::Vector<Dimensions,double>&  particleX
      );


      /**
       * Similar to mapParticleOntoVoxel() but this time, we always take the
       * biased voxel to the left, i.e. we always use the floor and look
       * left.
       */
      tarch::la::Vector<Dimensions,int> mapBiasedParticleOntoVoxel(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        const tarch::la::Vector<Dimensions,double>&  particleX
      );


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
      double projectValueOntoParticle_piecewiseConstant(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const double* __restrict__                   Q,
        const tarch::la::Vector<Dimensions,double>&  particleX,
        int                                          unknown
      );


      double projectValueOntoParticle_piecewiseLinear(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const double* __restrict__                   Q,
        const tarch::la::Vector<Dimensions,double>&  particleX,
        int                                          unknown
      );
    }
  }
}


#include "Tracer.cpph"

