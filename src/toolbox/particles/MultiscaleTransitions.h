// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <tuple>
#include "peano4/datamanagement/VertexMarker.h"


namespace toolbox {
  namespace particles {
    /**
     * @page MultilevelSorting Multilevel sorting tolerance
     *
     * As the particle association is not unique and not sharp, as we work with
     * floating point numbers. The correlation
     *
     * @f$ centre_{left} + h/2 < centre_{left} - h/2 @f$
     *
     * might not hold for two neighbouring voxels, and notably there might be a
     * gap in-between two voxels. If particles fall into this gap, they would
     * not belong to any voxel. We therefore give each voxel a small overlap.
     * The relative size of this overlap is quantified by SpatialSortingTolerance.
     * This is out first principle: particles are always assigned to a cell/volume
     * relative to a relative threshold. We issue a resort if and only if the
     * particle is outside of its respective control volume subject to the
     * additional tolerance.
     *
     * There is a second threshold: Our sorting should be idempotent. That is,
     * once we assign a particle to a vertex or cell, we should not immediately
     * require yet another resorting step.
     *
     * The two principles imply that we work with two thresholds: We trigger a
     * resort if a particle leaves its control volume. When we re-assign a
     * particle into a control volume, we have to be careful: we have to be on
     * the save side and be sure that this re-assignment will, under no
     * circumstances, trigger a new resort.
     *
     * @image MultiscaleTransitions.png
     *
     * The sketch above illustrates this fact which essentially means that we
     * work with two tresholds: Green and blue are two cells. Each is
     * surrounded by a halo layer (grey). A particle is reassigned to another
     * cell if and only if it leaves the corresponding halo. So a particle
     * might start in the green cell and move to the right. But the green cell
     * will not trigger a resort unless the particle also leaves its halo.
     * This is kind of a push mechanism (get rid of a particle).
     *
     * If a particle is to be sorted globally, i.e. resides within a global
     * list and has to be sorted into a cells, we realise some kind of pull
     * mechanism. Individual cells grab particles. Same happens throughout
     * drops: cells grab particles from coarser levels. This happens if and
     * only if a particle would end up in the light blue or green,
     * respectively, domain. As this one is smaller than the grey halo, we
     * are on the safe side, i.e. we know that a particle pull (drop or
     * sieve) would not trigger a resort.
     *
     *
     * ## Related action sets
     *
     * The action set toolbox.particles.UpdateParallelState employs the
     * threshold to decide if a particle is local or not.
     *
     *
     * ## Quantities
     *
     * - It is an absolute tolerance.
     * - MultilevelSortingTolerance is the counterpart for multilevel
     *   transitions.
     * - AsymmetricSortingDamping defines the inner ratio relative to the
     *   tolerance.
     *
     *
     * @see AsymmetricSortingDamping
     * @see getParticleReassociationInstructionWithinCellWithIntraCellReassignment()
     */
    constexpr double SpatialSortingTolerance       = 1.01;

    /**
     * @see SpatialReassignmentTolerance
     */
    constexpr double MultilevelSortingTolerance    = 1.01;

    /**
     * We try to pick the damping such that the two halo layers from the
     * description of SpatialSortingTolerance are roughly of the same size.
     *
     * 0.5 ( 1.0+SpatialSortingTolerance ) = AsymmetricSortingDamping * SpatialSortingTolerance
     *
     * That is
     *
     * 0.5 + 0.5 * SpatialSortingTolerance = AsymmetricSortingDamping * SpatialSortingTolerance
     *
     * @see MultilevelSortingTolerance
     * @see SpatialSortingTolerance
     */
    constexpr double AsymmetricSortingDamping              = 0.5 / SpatialSortingTolerance + 0.5;


    /**
     * Default tolerance to decide when two particles are the same.
     *
     * @see eliminateSpatialDuplicates()
     */
    constexpr double SpatialDuplicateTolerance = 1e-6;

    typedef int ParticleReassociationInstruction;

    constexpr int ParticleReassociationInstruction_Keep          = -1;
    constexpr int ParticleReassociationInstruction_SieveGlobally = -2;

    /**
     * If you use this operation, you can be sure that every particle is
     * associated to the right vertex after we've finished the traversal, or
     * it is dumped into the set of particles that have to be sieved.
     * Therefore, you don't have to check any particle anymore in
     * touchVertexLastTime(). All should be in place.
     *
     * We return
     *
     * - ParticleReassociationInstruction_Keep if the particle should
     *   remain where it is.
     * - A value between 0 and 2^d-1 if we should assign it to another
     *   vertex of this cell (on the same level).
     *
     * ## Algorithm
     *
     * - If the particle is not local or actually not covered by the current
     *   cell, we return keep, as we don't dare to make a decision.
     * - When we identify the destination cell, we violate our halo discussion
     *   above, i.e. we move the association within a cell given harsh
     *   boundaries not subject to MultilevelSortingTolerance.
     *
     *
     */
    template <typename Particle>
    ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      const Particle&                            p,
      const peano4::datamanagement::CellMarker&  marker,
      int                                        numberOfVertexWithinCell
    );


    /**
     * Take particle from current vertex and lift and further
     *
     * Should be called by touchVertexLastTime if you decide to sort particles
     * in this routine and not within the cell. It returns one of the following
     * constants:
     *
     * - ParticleReassociationInstruction_Keep. All is fine, just analyse the
     *   next particles.
     * - 0..2^d-1. The particle should not be associated to this vertex. Remove
     *   the association and associate to the next coarsest level. The vertex
     *   within this coarse cell is identified by the result number.
     * - ParticleReassociationInstruction_SieveGlobally. Particle should not be
     *   associated with this vertex, but you can't just move it one level up.
     *   Remove it completely from mesh and push it into a global list of
     *   particles which shall be sieved in the next step.
     *
     * ## Algorithm
     *
     * If a particle is not local, we keep it, as we don't dare to make a
     * comment.
     *
     *
     * We basically first check if a particle is contained within the h/2
     * environment of a vertex. For this, we exploit the fact that a marker
     * has a routine x() which returns the position of the associated of the
     * vertex plus a routine h() which returns the size of the adjacent cells.
     *
     * If the particle is at the right place, we return ParticleReassociationInstruction_Keep.
     * Nothing is to be done. Otherwise, we have to lift:
     *
     * If the parent cell is local, then we can safely move the particles
     * up the tree, as we know that all 2^d coarse grid vertices are
     * adjacent to the local domain.
     *
     * If the parent cell is not local, we have
     * to assume that we just did hit a vertical tree cut. In this case,
     * we are on the safe side if we just add the particles to the global
     * sieve particle list. We return ParticleReassociationInstruction_SieveGlobally.
     *
     * All of these considerations apply if and only if a particle is local. If
     * a particle is virtual, it is a copy from another rank. In this case, we
     * should not change its position, and we should not change its association.
     *
     * @see LiftingTolerance
     */
    template <typename Particle>
    ParticleReassociationInstruction liftParticleAssociatedWithVertex(
      const Particle&                             p,
      const peano4::datamanagement::VertexMarker  marker
    );


    /**
     * Find out which vertex should hold a particle.
     *
     * @param particle The particle of interest to be studied.
     * @param marker   A description of the cell into which the particle is
     *                 currently inserted or shall be inserted.
     * @return         The number of the vertex within the cell identified by
     *                 marker to which you should assign particle. Use
     *                 to_ulong() to convert the result, as the result
     *                 identifies the vertex in z-order, i.e. if it returns 00
     *                 it is the left bottom vertex, 10 is the right bottom
     *                 one, and so forth.
     */
    template <typename Particle>
    std::bitset<Dimensions>  getParticleAssociationWithinCell(
      const Particle&                           particle,
      const peano4::datamanagement::CellMarker  marker
    );


    /**
     * A particle is to be sieved into a vertex if
     *
     * - this vertex is closer to the particle than half the mesh size of
     *   an adjacent mesh element, as we always store particles next to
     *   their closest vertex; AND
     * - this vertex's cut-off radius would not fit into the next finer
     *   mesh level OR
     *   there is no next finer mesh level; AND
     * - the particle's cut-off is not too big for the current mesh level.
     *
     *
     * ## Rationale and usage
     *
     * I assume that the tree sieving particles removes a particle from the
     * sieve set as soon as it is inserted into the domain. The association
     * of particles to vertices is not unique. In line with this, the sieving
     * is not unique either. So it is important that you remove the particle
     * from the sieve set once it is inserted.
     *
     * In theory, we could insert particles on the top level of a segment and
     * then rely on the drops to move them down. This all works fine in a
     * serial code. Once you parallelise, we however encounter an issue when
     * particles drop through vertical tree cuts, i.e. if the fine grid child
     * of a cell is remote. In this case, we might have inserted a particle
     * into a rather coarse resolution level and rely on the drop. The remote
     * child will insert the particle as well. The father will not be able to
     * drop the particle all the way down into a remote tree.
     *
     *
     *
     * When we sieve a particle into the next level, we have to find
     * out to which particle we do so. To find this out, we run over
     * the vertices as we encounter touchVertexFirstTime(). As particles
     * are not uniquely assigned to cells or vertices, we have some
     * flexibility here. There are two things to weight up:
     *
     * - We have to ensure that the particles actually go down;
     * - We have to ensure that we don't immediately lift the particle
     *   again.
     *
     * The latter property is essential: Usually, drops, sieves and
     * lifts are done within one action set. However, we always need
     * a second sweep after each lift. If the user runs two sweeps with
     * the action set, the users relies upon the fact that all is sorted
     * completely. If you lift again immediately due to an invalid drop,
     * we mess up this logic. Therefore, it is important that this
     * sorting tolerance is always smaller than the lift tolerance.
     *
     * We always try to sieve a particle directly into the right level.
     * Once more, it is absolutely essential that we don't sieve into
     * a cell or vertex where we then have to lift stuff again.
     *
     * Lifting will always be subject to a comparison similar to
     *
     *       marker.h() < 2.0 * particle.getCutOffRadius()
     *
     * This threshold is hard. However, we can sieve into a coarser
     * level obviously to ensure that we don't violate it.
     *
     *
     * @see sieveParticle()
     */
    template <typename Particle>
    bool sieveParticle(
      const Particle&                              particle,
      const peano4::datamanagement::VertexMarker&  marker
    );


    template <typename Particle>
    bool dropParticle(
      const Particle&                              particle,
      const peano4::datamanagement::VertexMarker&  marker
    );


    /**
     * Eliminate spatial duplicates
     *
     * Whenever we work with distributed data structures and multilevel transitions
     * in a distributed memory mindeset, we can run into redundant particles which
     * we then have to elimninate.
     *
     * @return [int,int] representing [numberOfLocalEliminatedSpatialDuplicates,numberOfVirtualEliminatedSpatialDuplicates]
     */
    template <typename ParticleContainer>
    std::tuple<int, int> eliminateSpatialDuplicates(
      ParticleContainer&   particleSet
    );


    double relativeSpatialSortingTolerance(
      const peano4::datamanagement::VertexMarker&  marker
    );
    double relativeSpatialSortingTolerance(
      const peano4::datamanagement::CellMarker&  marker
    );
    double relativeMultilevelSortingTolerance(
      const peano4::datamanagement::VertexMarker&  marker
    );
    double relativeMultilevelSortingTolerance(
      const peano4::datamanagement::CellMarker&  marker
    );
  }
}


#include "MultiscaleTransitions.cpph"
