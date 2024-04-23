// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <tuple>

#include "particles.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"


namespace toolbox {
  namespace particles {
    typedef int ParticleReassociationInstruction;

    constexpr int ParticleReassociationInstruction_Keep          = -1;
    constexpr int ParticleReassociationInstruction_SieveGlobally = -2;

    /**
     * Sorting tolerance
     *
     * As the particle association is not unique and not sharp, as we work with
     * floating point numbers. The correlation
     *
     * @f$ centre_{left} + h/2 < centre_{right} - h/2 @f$
     *
     * might not hold for two neighbouring voxels, i.e. there might be a floating-point
     * gap in-between two voxels. If particles fall into this gap, they would
     * not belong to any voxel. We therefore give each voxel a small overlap.
     * The relative size of this overlap is quantified by ReleaseOwnershipSpatialSortingTolerance.
     * This is out first principle: particles are always assigned to a cell/volume
     * relative to a relative threshold. We issue a resort if and only if the
     * particle is outside of its respective control volume subject to the
     * additional tolerance. If a particle leaves a cell or-vertex environment
     * subject to the ReleaseOwnershipSpatialSortingTolerance, we either assign it
     * to a neighbouring voxel or lift it into a coarser level or assign it to
     * a global sorting list.
     *
     * There is a second threshold: Our sorting should be idempotent. That is,
     * once we assign a particle to a vertex or cell, we should not immediately
     * require yet another resorting step. Two rules formalise this:
     *
     * 1. If a vertex releases a stationary particle, it will never grab it
     *    back.
     * 2. If a vertex grabs a stationary particle, it will never release it.
     *
     * The two principles imply that we work with two thresholds: We trigger a
     * resort if a particle leaves its control volume. The region in which we
     * grab a particle however is slightly smaller. We work with two
     * thresholds, which all span areas which are slighter than the control
     * volumes. However, releases are way more conservative/careful than grabs
     * or drops.
     *
     * @image MultiscaleTransitions.png
     *
     * ## Related action sets
     *
     * The action set peano4.toolbox.particles.api.UpdateParallelState employs the
     * threshold to decide if a particle is local or not. It has to use the
     * most generic environment, i.e. the release radius.
     *
     * @see AsymmetricSortingDamping
     */
    constexpr double ReleaseOwnershipSpatialSortingTolerance = 1.01;

    /**
     * @see SpatialReassignmentTolerance
     */
    constexpr double GrabOwnershipSpatialSortingTolerance = 1.001;

    /**
     * Take particle from current vertex and lift it or keep it local
     *
     * This routine should be called by touchVertexLastTime(). Different to
     * its cousin getParticleReassociationInstructionWithinCellWithIntraCellReassignment(),
     * it should not be called within the cell. Usually, users do not invoke
     * the routine directly. Instead the action set peano4.toolbox.particles.api.UpdateParticleGridAssociation_ListDrop
     * injects calls to this routine into the generated source code.
     *
     * The function returns one of the following values:
     *
     * - particles::internal::ParticleReassociationInstruction_Keep. All is fine, just analyse the
     *   next particles.
     * - particles::internal::ParticleReassociationInstruction_SieveGlobally. Particle should not be
     *   associated with this vertex, but you can't just move it one level up.
     *   Remove it completely from mesh and push it into a global list of
     *   particles which shall be sieved in the next step.
     * - A value between 0 and @f$ 2^d-1 @f$. In this case, the particle should not be associated to this vertex. Remove
     *   the association and associate to the next coarsest level. The vertex
     *   within this coarse cell is identified by the result number.
     *
     *
     * ## Algorithm
     *
     * The algorithm starts from a simple observation:
     *
     * - If a particle is not local, we keep it, as we don't dare to make a
     *   comment to which mesh entity is should be moved to. Actually,
     *   non-local particles should never be moved. Hence, we could run the
     *   association analysis and then check if a non-local particle is kept
     *   where it should be, but we do it the other way round: We don't
     *   continue to check for virtual particles. We use the local flag as
     *   an optimisation opportunity.
     * - If a particle is local, it might have moved and it might have to be
     *   assigned to another vertex.
     *
     * @image html MultiscaleTransitions_belongs-to-vertex.svg
     *
     * Particles are always associated to their closest vertex. If the blue
     * particle moves, it remains within that area around the vertex where it
     * belongs to, and we can return a particles::internal::ParticleReassociationInstruction_Keep
     * command. peano4::datatraversal::VertexMarker::isContainedInAdjacentCells()
     * allows us to make this decision quickly: We check if a particle is
     * contained within the h/2 environment around a vertex.
     * If a particle has left this
     * (greenish) area around a vertex, we have to associate it to another
     * vertex. We have to lift.
     * Lifts however can only be realised within the local spacetree, where we
     * have local and thread-safe access to the coarse level data structures of
     * the tree.
     *
     * liftParticleAssociatedWithVertex() is called by
     * touchVertexLastTime() which in turn is called from within a cell: We are
     * about to leave a cell, find out that a vertex won't be needed anymore,
     * and consequently trigger touchVertexLastTime().
     * Each cell has a unique parent within the spacetree.
     *
     * If the parent cell is not local, we have
     * to assume that we just did hit a vertical tree cut. In this case,
     * we are on the safe side if we just add the particles to the global
     * sieve particle list. We return ParticleReassociationInstruction_SieveGlobally.
     *
     * If the parent cell is local, we can determine which of the @f$ 2^d @f$
     * coarse grid vertices would hold the particle.
     * If this coarse grid is not adjacent to the local domain, we have to sieve.
     * In the illustration below, we can see this behaviour on the right-hand
     * side:
     *
     * @image html MultiscaleTransitions_liftParticleAssociatedWithVertex.png
     *
     * - The particle (red solid dot) moves from left to right.
     * - Due to the move, the particle's current vertex association (blurry
     *   blue) becomes invalid.
     * - We have would like to lift it to the red blurry vertex.
     * - The parent cell belongs to another rank (yellow instaed of blue).
     * - Therefore, we have to add it to the global sieve list.
     *
     * There is a second, more delicate case which is illustrated on the
     * left-hand side of the sketch:
     *
     * - Let the part move from left to right on the fine grid owned by
     *   the blue rank.
     * - Therefore, the particle changes its vertex association from
     *   blurry blue to blurry green.
     * - We cannot realise this reassociation directly within
     *   touchVertexLastTime(). Therefore, we lift the particle to the
     *   red coarse grid vertex.
     * - This vertex also is adjacent to the blue rank. Nevertheless, we
     *   have to assign is to the global sieve list, as we move it through
     *   the horizontal tree cut.
     *
     * If we reassigned it locally, it would be labelled as remote on the
     * coarser level in the next iteration. The yellow rank in return would
     * receive a copy from the blue-yellow coarse grid boundary and then
     * label it as local there. While this is correct, we now have to sieve
     * it through a horizontal tree cut again. All not nice. We better get
     * it straight and assign it to the global sieve list right from the start.
     *
     * The need for special care becomes apparent once we take into account
     * which cell triggers the touchVertexLastTime(): Should it be a cell
     * above the two fine grid vertices of interest, we would always associate
     * the particle to the global sieve set, as we spot the multilevel transition.
     * If it is called from one of the two adjacent cells below, the parent
     * cell is local and we cannot spot the tree cut.
     *
     * Originally, I thought we could get away with an analysis of the coarse
     * vertex and use particleAssignedToVertexWillBeLocal() to figure out
     * whether we lift thorough a tree cut or not. However, we do not have
     * the marker of the coarse grid vertices. So we cannot access the information
     * we'd need. So the only thing that we can do is to lift if and only if
     * we lift within the coarse cell and therefore can be sure that our
     * analysis of the parent cell's owner provides sufficient information.
     *
     * @see toolbox::particles::internal::relativeReleaseOwnershipSpatialSortingTolerance()
     */
    template <typename Particle>
    ParticleReassociationInstruction liftParticleAssociatedWithVertex(const Particle& p, const peano4::datamanagement::VertexMarker marker);

    /**
     * Implementation of liftParticleAssociatedWithVertex() without the template
     */
    ParticleReassociationInstruction liftParticleAssociatedWithVertex(
      bool isLocal, double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker marker
    );

    /**
     * A particle is to be sieved into a vertex if
     *
     * - this vertex is closer to the particle than half the mesh size of
     *   an adjacent mesh element, as we always store particles next to
     *   their closest vertex; AND
     * - this vertex's search radius would not fit into the next finer
     *   mesh level OR
     *   there is no next finer mesh level; AND
     * - the particle's search radius is not too big for the current mesh level.
     *
     * This routine does not check in any way if the particle will be local.
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
     *       marker.h() < 2.0 * particle.getSearchRadius()
     *
     * This threshold is hard. However, we can sieve into a coarser
     * level obviously to ensure that we don't violate it.
     *
     * ## Optimisation
     *
     * I started off with this nice simple code
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  const bool   fitsIntoThisGridResolution      = tarch::la::min( marker.h() )     * MultilevelSortingTolerance *
     * AsymmetricSortingDamping >= 2.0 * particle.getSearchRadius(); const bool   fitsIntoNextFinerGridResolution =
     * tarch::la::min( marker.h() )/3.0 >  2.0 * particle.getSearchRadius();
     *
     *  return marker.isContainedInAdjacentCells( particle.getX(), 0.5, relativeReleaseOwnershipSpatialSortingTolerance(marker) *
     * AsymmetricSortingDamping * tarch::la::NUMERICAL_ZERO_DIFFERENCE) and fitsIntoThisGridResolution and ( not
     * fitsIntoNextFinerGridResolution or not marker.hasBeenRefined()
     *     );
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * However, it turns out a lot of runtime is spent in getSearchRadius(). It
     * is not clear if the compiler manages to exploit the non-strict evaluation
     * if we first define the two const bools. So I moved them now into the
     * boolean expression, i.e. work without the explicit temporary variables.
     *
     * @see sieveParticle()
     */
    template <typename Particle>
    bool sieveParticle(const Particle& particle, const peano4::datamanagement::VertexMarker& marker);

    bool sieveParticle(double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker& marker);

    template <typename Particle>
    bool dropParticle(const Particle& particle, const peano4::datamanagement::VertexMarker& marker);

    /**
     * Alternative version of dropParticle() without templates
     *
     * This is the actual implementation, i.e. the template delegates to this
     * one. Through the method, we can unittest the drop logic.
     */
    bool dropParticle(double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker& marker);

    /**
     * Will the particle be dropped further throughout the traversal
     *
     * The sorting in Peano usually realises pull semantics. That is, the fine
     * grid vertex pulls particles down from coarser levels into its own
     * domain. Particles recursively drop down the resolution cascade.
     * Throughout the top-down tree traversal, it is often important to know if
     * a particle will be pulled down further later on throughout the mesh
     * traversal. This routine tells the user if this will happen. It is
     * somehow the top-down counterpart of dropParticle() and sieveParticle().
     *
     * Consult @ref page_toolbox_particles_mesh_traversal "the generic mesh traversal discussion"
     * for some further "use cases" where this operation becomes absolutely
     * mandatory.
     *
     * ## Realisation
     *
     * There are two major checks which determine if a particle will be
     * dropped further or not: The mesh size vs the cut-off radius of a
     * particle, and the mesh cell state. If a mesh cell is not refined
     * and will not be refined throughout this very traversal, then the
     * particle will also not be dropped. It will however be dropped if
     * it resides within a cell that's already refined or will be refined
     * in this mesh sweep, and the cut-off radius will also fit into the
     * next finer level.
     *
     * A cell is refined or will be refined
     *
     * - if peano4::datamanagement::CellMarker::hasBeenRefined() holds,
     *   as this means that this cell actually already has children; or
     * - if peano4::datamanagement::CellMarker::willBeRefined() holds,
     *   as this means that this cell either has children already or will
     *   be added children in this very mesh sweep.
     *
     * We note that hasBeenRefined() can be wrong and willBeRefined() might
     * hold. In this case, the new children in the tree are added in this
     * very mesh traversal. It can also happen that hasBeenRefined() holds
     * and willBeRefined() is wrong. In this case, this very mesh traversal
     * will eliminate the children, i.e. coarsen. Nevertheless, the
     * particles still are to be dropped into their respective mesh level -
     * even though we might then lift them straightaway again as we coarsen.
     *
     *
     * ## Usage in combination with particle-particle interactions
     *
     * Deciding whether to update a particle within a mesh sweep
     * that also introduces drops is delicate: Quickly, we run into situations
     * where particles are updated multiple times. The documentation of
     * peano4.toolbox.particles.api.AbstractUpdateParticleGridAssociation
     * provides examples for this.
     *
     */
    template <typename Particle>
    bool particleWillBeDroppedFurther(const Particle& particle, const peano4::datamanagement::CellMarker& marker);

    bool particleWillBeDroppedFurther(double searchRadius, const peano4::datamanagement::CellMarker& marker);

    /**
     * Will a particle be dropped further
     *
     * A a first glance, this routine seems to be a replica of dropParticle().
     * However, this is not the case. dropParticle() is asked from a child for
     * a parent vertex if it shall drop a particle from the parent into this
     * particular child vertex. This routine is called for a plain vertex
     * without any multiscale relation and tells the callee if a vertex
     * holding this particular particle will later on be stipped off the
     * particle.
     */
    template <typename Particle>
    bool particleWillBeDroppedFurther(const Particle& particle, const peano4::datamanagement::VertexMarker& marker);

    bool particleWillBeDroppedFurther(double searchRadius, const peano4::datamanagement::VertexMarker& marker);

    /**
     * Ownership tolerance
     *
     * A particle is "owned" by a cell (or vertex, respectively), if it resides
     * within its area. However, we slightly augment this area, and this
     * tolerance reflects the augmentation. It basically invokes
     * internal::relativeReleaseOwnershipSpatialSortingTolerance() and
     * multiply it with the numerical zero
     * tarch::la::NUMERICAL_ZERO_DIFFERENCE.
     */
    double relativeSpatialOwnershipTolerance(const ::peano4::datamanagement::CellMarker& marker);

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
      const Particle& p, const peano4::datamanagement::CellMarker& marker, int numberOfVertexWithinCell
    );

    ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      bool isLocal, double searchRadius, const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::CellMarker& marker, int numberOfVertexWithinCell
    );

    /**
     * Find out which adjacent cell will hold a particle.
     *
     * Let the routine find out which of the neighbouring cells will try to
     * claim ownership of a particle. In exact arithmetics, this routine would
     * be the same as internal::getParticleAssociationWithinCell(). However, we
     * work with floating point precision and cells with a slight overlap.
     * Therefore, multiple adjacent cells might try to claim ownership of the
     * particle potentially, and this is important to find out a priori whether
     * a particle will be labelled as inside or not.
     *
     * For the floating point reason, i.e. the fact that the particle ownership
     * is not unique, VertexMarker::getRelativeAdjacentCell() is a more strict
     * version of this routine and cannot be used in some cases.
     *
     * @param particle The particle of interest to be studied.
     * @param x        Vertex centre.
     */
    std::bitset<TwoPowerD> getAdjacentCellsOwningParticle(const tarch::la::Vector<Dimensions, double>& x, const peano4::datamanagement::VertexMarker& marker);

    namespace internal {
      double relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker);
      double relativeReleaseOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker);
      double relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::VertexMarker& marker);
      double relativeGrabOwnershipSpatialSortingTolerance(const ::peano4::datamanagement::CellMarker& marker);

      bool fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::CellMarker& marker);
      bool fitsIntoLevel(double searchRadius, const ::peano4::datamanagement::VertexMarker& marker);

      /**
       * Find out which vertex should hold a particle.
       *
       * @param particle The particle of interest to be studied.
       * @param cellCentre   Results from CellMarker::x().
       * @return         The number of the vertex within the cell identified by
       *                 marker to which you should assign particle. Use
       *                 to_ulong() to convert the result, as the result
       *                 identifies the vertex in z-order, i.e. if it returns 00
       *                 it is the left bottom vertex, 10 is the right bottom
       *                 one, and so forth.
       */
      std::bitset<Dimensions> getParticleAssociationWithinCell(const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& cellCentre);
    } // namespace internal
  }   // namespace particles
} // namespace toolbox

#include "MultiscaleTransitions.cpph"
