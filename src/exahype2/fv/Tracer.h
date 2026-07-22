// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "PatchUtils.h"

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"

namespace exahype2 {
  namespace fv {
    /**
     * Project one quantity from the patch data onto the particle
     *
     * The quantities of interest are given as template parameters, i.e.
     * through SourceIndex and DestIndex. ExaHyPE's tracing expects a plain
     * function name to project the solution onto a particle, so the source
     * and the destination index have to be baked into the type.
     *
     * The usage of this routine within exahype2.tracer.FiniteVolumesTracing
     * is trivial: Pass in
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * project_on_tracer_properties_kernel="::exahype2::fv::projectValueOntoParticle_piecewiseConstant<3,2>"
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * if you want to map the fourth entry of the solution onto the third one of
     * the tracer.
     *
     * @param marker           Cell marker describing the cell's geometric and
     *   spatial properties.
     * @param voxelsPerAxis    Number of finite volumes per coordinate axis. We
     *   assume that the patch has no halo.
     * @param unknownsPerVoxel Number of entries each finite volume holds.
     * @param Q                Voxel field, i.e. actual patch data. Has the
     *   dimensions @f$ voxelsPerAxis^d \cdot unknownsPerVoxel @f$.
     * @param particle         Particle which receives the value. The routine
     *   reads its position and writes the entry DestIndex.
     */
    template <int SourceIndex, int DestIndex, typename Particle, typename DataType>
    void projectValueOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      Particle&                                 particle
    );

    /**
     * Project all values from solution onto particle
     *
     * This routine assumes that the particle hosts exactly the same number
     * of unknowns as you store within the solution, i.e. within each entry
     * of Q. The count refers to the unknowns plus auxiliary variables within
     * ExaHyPE. If you host a PDE with 10 unknowns and 3 material parameters,
     * then this routine works only when the particle carries 13 unknowns.
     *
     * The routine assumes that Q represents a piecewise constant solution
     * and consequently maps the data piecewise constant. This holds for
     * Finite Volumes, e.g., where each unknown within Q stores data in the
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
     * The usage of this routine within exahype2.tracer.FiniteVolumesTracing
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
    template <typename Particle, typename DataType>
    void projectAllValuesOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      Particle&                                 particle
    );

    template <int SourceIndex, int DestIndex, typename Particle, typename DataType>
    void projectValueOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      Particle&                                 particle
    );

    /**
     * Project all values from solution onto particle and use a piecewise linear interpolation
     *
     * @see projectAllValuesOntoParticle_piecewiseConstant()
     */
    template <typename Particle, typename DataType>
    void projectAllValuesOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      Particle&                                 particle
    );

    /**
     * Project all values from the solution onto the particle with a piecewise linear
     * interpolation
     *
     * This is the explicit-Euler variant of projectAllValuesOntoParticle_piecewiseLinear().
     * It additionally accepts the time step size, a set of DIMENSIONS indices naming the
     * particle entries that hold the velocity, and a set of scale factors for these
     * velocities. Enter {-1,-1,-1} as factors, e.g., if the velocity is the negative of
     * the indexed quantities.
     *
     * The routine maps the patch data onto the particle attributes. The time step
     * size, the indices and the factors are part of the signature, but the Euler
     * update of the particle velocity is not evaluated.
     *
     * To use this extended mapping, you have to use the Python API similar to the
     * following snippet:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * tracing_action_set = exahype2.tracer.FiniteVolumesTracing(tracer_particles,
     *   self,
     *   project_on_tracer_properties_kernel="projectAllValuesOntoParticle_piecewiseLinear_explicit_Euler",
     *   projection_kernel_arguments="""
     *     marker,
     *     {{PATCH_SIZE}},
     *     {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
     *     fineGridCell{{SOLVER_NAME}}Q.data(),
     *     fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
     *     {16,17,18},
     *     {1,1,1},
     *     *p
     *   """
     *   )
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    template <typename Particle, typename DataType>
    void projectAllValuesOntoParticle_piecewiseLinear_explicit_Euler(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      double                                    timeStepSize,
      tarch::la::Vector<DIMENSIONS, int>        indices,
      tarch::la::Vector<DIMENSIONS, double>     factors,
      Particle&                                 particle
    );

    /**
     * Map multiple variables from the input field onto the particle
     *
     * The usage of this routine within exahype2.tracer.FiniteVolumesTracing
     * is simplistic due to the variadic template:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * project_on_tracer_properties_kernel="::exahype2::fv::projectValuesOntoParticle_piecewiseConstant<double, 4, 5, 9>"
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * names the entries 4, 5 and 9 of the solution field, so the particle has
     * to carry at least three unknowns. The helper in the internal namespace
     * writes the first entry of the particle; the further indices of the pack
     * are accepted but not evaluated.
     */
    template <typename DataType, int SourceIndex, int... SourceIndices, typename Particle>
    void projectValuesOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      Particle&                                 particle
    );

    template <typename DataType, int SourceIndex, int... SourceIndices, typename Particle>
    void projectValuesOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      Particle&                                 particle
    );

    /**
     * Advance one particle by a single time step
     *
     * The routine interpolates the patch data onto the particle through
     * projectValuesOntoParticle_piecewiseLinear(), writes a constant gravitational
     * acceleration into the particle's acceleration entries, derives the new position
     * with explicitEulerIntegration(), reflects the particle at the domain boundary,
     * and finally commits the position.
     *
     * @param particle         Particle whose data, velocity and position are updated.
     * @param timeStepSize     Time step size used by the Euler update.
     * @param marker           Cell marker describing the cell's geometric and spatial
     *   properties.
     * @param voxelsPerAxis    Number of finite volumes per coordinate axis.
     * @param unknownsPerVoxel Number of entries each finite volume holds.
     * @param Q                Voxel field, i.e. actual patch data.
     * @param DomainSize       Extent of the computational domain.
     * @param DomainOffset     Bottom left front corner of the computational domain.
     * @param PeriodicBC       Per-axis periodicity flags. The reflection at the domain
     *   boundary runs if at least one axis carries a one.
     */
    template <int SourceIndex, int... SourceIndices, typename Particle, typename DataType = double>
    void updateParticleState(
      Particle&                                 particle,
      double                                    timeStepSize,
      const peano4::datamanagement::CellMarker& marker,
      int                                       voxelsPerAxis,
      int                                       unknownsPerVoxel,
      const DataType* NOALIAS                   Q,
      tarch::la::Vector<DIMENSIONS, double>     DomainSize,
      tarch::la::Vector<DIMENSIONS, double>     DomainOffset,
      tarch::la::Vector<DIMENSIONS, double>     PeriodicBC
    );

    /**
     * Perform an explicit Euler integration step
     *
     * The velocity is updated first and clamped to a fixed range; the new position
     * then follows from the updated velocity and the acceleration. Velocity and
     * acceleration live in the particle's data entries 3+d and 5+d.
     *
     * @param particle     Particle whose velocity entries are updated in place.
     * @param timeStepSize Time step size used in the integration.
     * @return The particle position after one explicit Euler step.
     */
    template <typename Particle>
    tarch::la::Vector<DIMENSIONS, double> explicitEulerIntegration(Particle& particle, double timeStepSize);

    /**
     * Reflect a particle that has left the domain
     *
     * A position outside the domain is mirrored back at the boundary it crossed, and
     * the velocity component along that axis is reversed and damped, so the particle
     * stays inside the domain and loses part of its energy on impact.
     *
     * @param particle     Particle whose velocity entries are updated in place.
     * @param newPosition  Candidate position. Components outside the domain are
     *   replaced by their mirrored counterparts.
     * @param DomainSize   Extent of the computational domain.
     * @param DomainOffset Bottom left front corner of the computational domain.
     */
    template <typename Particle>
    void handleBoundaryCollisions(
      Particle&                              particle,
      tarch::la::Vector<DIMENSIONS, double>& newPosition,
      tarch::la::Vector<DIMENSIONS, double>  DomainSize,
      tarch::la::Vector<DIMENSIONS, double>  DomainOffset
    );

    namespace internal {
      /**
       * Assume that we have a particle suspended in a cell. The cell hosts a
       * regular Cartesian mesh. The routine computes the correct voxel.
       *
       * You can convert the result via
       *
       *       int voxelIndex = peano4::dLinearised(voxel,voxelsPerAxis);
       *
       * into an index to access your Q array. The whole construct assumes that
       * Q is not surrounded by a halo layer. If you have a halo layer, you have
       * to add (1,1,1) to the result and then increment voxelsPerAxis by two.
       */
      tarch::la::Vector<DIMENSIONS, int> mapParticleOntoVoxel(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        const tarch::la::Vector<DIMENSIONS, double>& particleX
      );

      /**
       * Similar to mapParticleOntoVoxel() but this time, we always take the
       * biased voxel to the left. Let a 1d patch be divided into voxels of
       * size h. Up to 1.5h, the operation returns 0. From 1.5h-2.5h, the
       * function yields a 1. So we always are biased towards the left
       * neighbour, which is useful whenever we use the outcome as input for a
       * linear interpolation. In this context, the result is also bounded by
       * 0 to the bottom (which is a natural choice), but no component of the
       * result vector exceeds voxelsPerAxis-2. You can always increment an
       * entry safely by a one.
       *
       * @see projectValueOntoParticle_piecewiseLinear() for an example where
       *   this helper routine is used
       */
      tarch::la::Vector<DIMENSIONS, int> mapBiasedParticleOntoVoxel(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        const tarch::la::Vector<DIMENSIONS, double>& particleX
      );

      /**
       * Project one quantity from the patch data onto the particle
       *
       * @param marker           Cell marker describing the cell's geometric and
       *   spatial properties.
       * @param voxelsPerAxis    Number of finite volumes per coordinate axis. We
       *   assume that the patch has no halo.
       * @param unknownsPerVoxel Number of entries each finite volume holds.
       * @param Q                Voxel field, i.e. actual patch data. Has the
       *   dimensions @f$ voxelsPerAxis^d \cdot unknownsPerVoxel @f$.
       * @param particleX        Position of the particle.
       * @param unknown          Which unknown from the data field to pick.
       */
      template <typename DataType>
      DataType projectValueOntoParticle_piecewiseConstant(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const DataType* NOALIAS                      Q,
        const tarch::la::Vector<DIMENSIONS, double>& particleX,
        int                                          unknown
      );

      /**
       * @see mapBiasedParticleOntoVoxel() which provides the index
       *   calculations, i.e. the code to identify the voxels which
       *   affect the particle of interest.
       */
      template <typename DataType>
      DataType projectValueOntoParticle_piecewiseLinear(
        const peano4::datamanagement::CellMarker&    marker,
        int                                          voxelsPerAxis,
        int                                          unknownsPerVoxel,
        const DataType* NOALIAS                      Q,
        const tarch::la::Vector<DIMENSIONS, double>& particleX,
        int                                          unknown
      );

      /**
       * Actual realisation of the projection routine. Has to be in a
       * subnamespace, as we have to permute the template arguments such
       * that the variadic arguments come last. Otherwise, C++ cannot match
       * them.
       */
      template <typename DataType, typename Particle, int SourceIndex, int... SourceIndices>
      void projectValuesOntoParticle_piecewiseConstant(
        const peano4::datamanagement::CellMarker& marker,
        int                                       voxelsPerAxis,
        int                                       unknownsPerVoxel,
        const DataType* NOALIAS                   Q,
        Particle&                                 particle,
        int                                       destinationIndex
      );

      template <typename DataType, typename Particle, int SourceIndex, int... SourceIndices>
      void projectValuesOntoParticle_piecewiseLinear(
        const peano4::datamanagement::CellMarker& marker,
        int                                       voxelsPerAxis,
        int                                       unknownsPerVoxel,
        const DataType* NOALIAS                   Q,
        Particle&                                 particle,
        int                                       destinationIndex
      );
    } // namespace internal
  } // namespace fv
} // namespace exahype2

#include "Tracer.cpph"
