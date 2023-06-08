// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"


namespace exahype2 {
  namespace fv {
    /**
     * Project one quantity from the patch data onto the particle
     *
     * The quantities of interest are given as template parameters, i.e.
     * through SourceIndex and DestIndex. This sounds weird (and likely is a
     * strange choice), but some of ExaHyPE's projection routines want to
     * have a plain function call to project the solution onto a particle,
     * and adding the source and destination index as template argument
     * allows us to have exactly this behaviour.
     *
     * The usage of this routine within the exaype2.tracers.FiniteVolumesTracing
     * is trivial: Pass in
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * project_on_tracer_properties_kernel="::exahype2::fv::projectValueOntoParticle_piecewiseConstant<3,2>"
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * if you want to map the fourth entry of the solution onto the third one of
     * the tracer, e.g.
     *
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
    template <int SourceIndex, int DestIndex, typename Particle>
    void projectValueOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    /**
     * Project all values from solution onto particle
     *
     * This routine assumes that the particle hosts exactly the same number
     * of unknown as you store within the solution, i.e. within each entry
     * of Q. The count refers to the unknowns plus auxiliary variables within
     * ExaHyPE. If you host a PDE with 10 unknowns and 3 material parameters,
     * then this routine works if and only if the particle carries 13 unknowns.
     *
     * The solution assumes that the Q represents a piecewise constant solution
     * and consequently maps the data piecewise constant. This holds for
     * Finite Volumes, e.g., where each unkonwn within Q stores data in the
     * AoS format and represents all the values within that finite volume. As
     * the FV scheme is 0th order, the data within the finite volume can be
     * considered to be piecewise constant, and the piecewise constant
     * interpolation hence makes sense. You might however decide that you
     * prefer a higher order interpolation scheme. In this case, consult
     * projectAllValuesOntoParticle_piecewiseLinear(), e.g.
     *
     * Some codes do not track all the unknowns via the tracers. In this case,
     * you have to use specialised routines to map the solution onto the
     * particle such as projectValueOntoParticle_piecewiseConstant() or
     * projectValuesOntoParticle_piecewiseConstant().
     *
     * The usage of this routine within the exaype2.tracers.FiniteVolumesTracing
     * is trivial: Pass in
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * as argument, and all is done. The routine here is a template, but the
     * template type will be induced by C++ automatically through the
     * function parameters.
     */
    template <typename Particle>
    void projectAllValuesOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    template <int SourceIndex, int DestIndex, typename Particle>
    void projectValueOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    /**
     * Project all values from solution onto particle and use a piecewise linear interpolation
     *
     * @see projectAllValuesOntoParticle_piecewiseConstant()
     */
    template <typename Particle>
    void projectAllValuesOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    /**
     * Map multiple variables from input field onto particle
     *
     *
     * The usage of this routine within the exaype2.tracers.FiniteVolumesTracing
     * is simplistic due to the variadic template:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * project_on_tracer_properties_kernel="::exahype2::fv::projectValuesOntoParticle_piecewiseLinear<4, 5, 9>"
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * maps the arguments 4, 5 and 9 from the solution field onto the tracer.
     * Hence, the function implicitly assumes that the particle has (at least)
     * three unkowns.
     */
    template <int SourceIndex, int... SourceIndices, typename Particle>
    void projectValuesOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* __restrict__                   Q,
      Particle&                                    particle
    );


    template <int SourceIndex, int... SourceIndices, typename Particle>
    void projectValuesOntoParticle_piecewiseLinear(
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


      /**
       * Actual realisation of the projection routine. Has to be in a
       * subnamespace, as we have to permute the template arguments such
       * that the variadic arguments come last. Otherwise, C++ cannot match
       * them.
       */
      template <typename Particle, int SourceIndex, int... SourceIndices>
      void projectValuesOntoParticle_piecewiseConstant(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const double* __restrict__                   Q,
        Particle&                                    particle,
        int                                          destinationIndex
      );


      template <typename Particle, int SourceIndex, int... SourceIndices>
      void projectValuesOntoParticle_piecewiseLinear(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const double* __restrict__                   Q,
        Particle&                                    particle,
        int                                          destinationIndex
      );

      /**
       * End-point of variadic templates and therefore nop.
       */
      template <typename Particle>
      void projectValuesOntoParticle_piecewiseConstant(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const double* __restrict__                   Q,
        Particle&                                    particle,
        int                                          destinationIndex
      );


      /**
       * End-point of variadic templates and therefore nop.
       */
      template <typename Particle>
      void projectValuesOntoParticle_piecewiseLinear(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const double* __restrict__                   Q,
        Particle&                                    particle,
        int                                          destinationIndex
      );
    }
  }
}


#include "Tracer.cpph"

