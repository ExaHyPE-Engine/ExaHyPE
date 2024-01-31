# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4


from peano4.solversteps import Steps
from swift2.actionsets.DynamicMeshRefinementAnalysis import (
    DynamicMeshRefinementAnalysis,
)
from swift2.actionsets.ScatterGlobalMemory import (
    ScatterGlobalMemory
)
from swift2.actionsets.DynamicMeshRefinementTrigger import DynamicMeshRefinementTrigger


def add_dynamic_mesh_refinement_and_particle_resorting(
    steps, 
    alter_position, 
    species_sets, 
    sort_with_lifts_and_drops,
    verbose
):
    """!

    Add action sets to step (aka mesh traversal) to adopt mesh to particle configuration


    The AMR logic is based upon the observation that Peano's toolbox always
    holds the particles in the right place and hence allows us to realise
    all particle-particle interactions. If we don't get the mesh right, we
    might not be optimal in terms of cost. But we are never wrong!

    ## Straightforward workflow

    In principle, SPH's AMR logic is pretty simple in the context of a the dual
    tree with its lifts and drops. The illustration below shows this trivial
    workflow on the left side:

    @image html AMR-logic.png

    The image is the time axis and the bullets are the individual mesh
    traversals. On the left, we see the user input which eventually triggers a
    cascade of actions on the bookkeeping side (right). The user alters the
    particle position and hence might invalidate the particle-grid association.
    We have to re-sort. Some sorting can happen right in the same mesh traversal
    in which we alter the particles' properties in the touchCellLastTime or
    touchVertexLastTime actions. The green colour denotes stuff that's happening
    in the last data access - which might also introduce some MPI sends.

    In the subsequent mesh traversal, we have to receive MPI data, move particles
    down in their tree and associate that to the correct vertex. This happens
    prior to any operation (denoted blue). Now that particles are in the right
    place, we can update their parallel state.

    A straightforward implementation now might evaluate the AMR criterion, as
    all particles are in place and we can do so. Once we have collected all AMR
    info, we can distribute it globally and hand it out in the subsequent mesh
    traversal in which we consequently trigger refinements. Peano does not
    realise refinements immediately, but waits for the next mesh traversal to
    do so.

    When we have done the refinement, we have to resort. Again, the resort has
    to spread over two mesh traversals, as the first half of the sorting might
    trigger MPI exchanges and multilevel traversals.

    In this native data flow scheme, the user action affects the behaviour of
    five follow-up mesh traversals.


    Alternative to add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption()
    which is compatible with memory pool solutions, as long as you insert a dummy mesh sweep
    after each routine which changes the particle-mesh topology (or you ensure that the
    subsequent sweep does not evaluate particle-particle interactions).

    The big difference to add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption()
    is that we do analyse the AMR after we've resorted the particles. However, we
    do not realise the AMR up until we have to due to physics. So we make the AMR
    lag behind even further.

    The big differences hence arise in the section where we do the association, and
    then we introduce the additional gather routines. As we do not update the
    association four times but only twice, we can also update the state only once
    instead of two times.

    ## Workflow optimised towards minimal resorting

    The optimised workflow accepts that we we are fine to deal with a suboptimal
    mesh layout, as long as we minimise the number of particle movements. This
    is a different cost objective than in the description above. We realise that
    we will always need a resort when we have updated a mesh.

    The idea is that we know that a user interaction forces us to update the
    particle-mesh association, while AMR does not: We have to lift particles
    from areas of the tree which are coarsened, but otherwise AMR does not
    invalidate a valid mesh association. Particles might reside on too coarse
    levels, but still all the kernel evaluations remain correct. The lift is the
    only thing that causes issues, as it might mean that particles literally
    travel.

    To avoid that we have to do two sorts per particle position update, we
    evaluate the AMR criterion two grid sweeps ahead of the actual particle
    position update. This way, the AMR realisation and the actual position
    update coincide in the same mesh sweep. No need for an additional sorting
    step.

    The disadvantage of this scheme is that the mesh always lags behind. The
    AMR just is about the realise when the particles move again. So it might
    have been optimal before, but now it is not good anymore. In return, we
    however get a scheme which does, amortised, need only one resorting step
    per mesh update plus AMR update.


    ## Interplay with gathering

    The aggressive AMR is not compatible with particles that gather data in memory
    pools.

    ## Realisation

    This implementation puts emphasis that the mesh is always as tighly
    adopted to the particles as possible. If necessary, this might mean
    that we resort the particles multiple times per grid sweep: If they
    change their position, we resort. But once we have resorted, we might
    want to change the mesh, which in turn means we have to resort again,
    as we might have new cells or cells might have been deleted.


    ## Order of action sets

    As so often, the order of the action sets is tremendously important,
    as the individual action sets interact with each other. When we
    discuss the order here, we may assume that all the function steps
    aka action sets are already fully implemented and in the right order.

    The trigger the AMR can be integrated whereever we want within the
    pipeline. It simply deploys events, i.e. it is order-agnostic.

    For the remaining action sets, we observe the following properties:

    - Mesh analysis: Looks into the cell stats. These are in place, so
      it should be one of the last things. The analysis happens in
      touchCellLastTime().
    - UpdateParallelState sets the particle flags in touchCellFirstTime().
      In the last access to the vertex, it looks at all particles' flags
      and throws away particles which are virtual. It is iportant that
      that flag update happens when all particles are properly in place.
      The throwing away should be the very last thing, though we can
      throw away particles before we move them around if we want.
    - UpdateAssociation. Should be the very first thing, but should
      happen, before we update the state. In principle, the state can
      also come later, but we might mess up the stats if it comes
      before the particle migration: it might count particles multiple
      times.

    So we want to have the following lifecycle from a grid's point of
    view:

    - touchVertexFirstTime():
      - Sort in the particles from coarser meshes or the global sieve list.
      - Set their new parallel state.
      - Then the user-defined stuff.
    - touchCellFirstTime():
      - Mark local particles as local, but it doesn't really matter when
        this happens.
    - touchCellLastTime():
      - User code.
      - Maintain some mesh statistics.
    - touchVertexLastTime():
      - User-defined stuff.
      - Resort particles (to coarser levels, e.g.).
      - Throw away virtual (remote) particles.


    ## Arguments

    @param steps: [peano4.solversteps.Step]

    @param alter_position: [{True,False}]

    @param species_sets: [Species]

    @param verbose: {True, False}
      Controls how much debug information is dropped.

    """
    if len(steps) != len(alter_position):
        raise Exception(
            "number of steps does not equal number of alter_position flags: {} vs {}".format(
                steps, alter_position
            )
        )

    for current_species_set in species_sets:
        global_steps_to_evaluate_amr = [False for i in range(0, len(steps))]
        global_steps_to_update_particle_grid_assocation = [
            False for i in range(0, len(steps))
        ]
        global_steps_to_update_parallel_state = [False for i in range(0, len(steps))]
        global_steps_to_realise_amr = [False for i in range(0, len(steps))]

        for i in range(0, len(steps)):
            if alter_position[i]:
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: step {} alters the particle positions of species {}".format(
                            i, current_species_set.name
                        )
                    )

                #
                #   AMR
                # =======
                #
                steps_to_evaluate_amr = {(i + 1) % len(steps)}
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: steps {} have to evaluate the AMR criterion".format(
                            steps_to_evaluate_amr
                        )
                    )
                for j in steps_to_evaluate_amr:
                    global_steps_to_evaluate_amr[j] = True

                #
                #   Association
                # ==============
                #
                steps_to_update_particle_grid_assocation = {
                    (i + 0) % len(steps),
                    (i + 1) % len(steps),
                }
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: steps {} have to update the particle-grid association of species {}".format(
                            steps_to_update_particle_grid_assocation,
                            current_species_set.name,
                        )
                    )
                for j in steps_to_update_particle_grid_assocation:
                    global_steps_to_update_particle_grid_assocation[j] = True

                #
                #   State
                # =========
                #
                # Action set peano4.toolbox.UpdateParallelState also eliminates 
                # halo values and therefore has to be invoked within each drift, 
                # too
                #
                steps_to_update_parallel_state = {
                    (i + 0) % len(steps),
                    (i + 1) % len(steps),
                }
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: steps {} have to update the parallel state of species {}".format(
                            steps_to_update_parallel_state, current_species_set.name
                        )
                    )
                for j in steps_to_update_parallel_state:
                    global_steps_to_update_parallel_state[j] = True

                #
                #   Realise AMR
                # ===============
                #
                #steps_to_realise_amr = {(i + 2) % len(steps)}
                steps_to_realise_amr = {(i - 1) % len(steps)}
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: steps {} have to realise the AMR instructions".format(
                            steps_to_realise_amr
                        )
                    )
                for j in steps_to_realise_amr:
                    global_steps_to_realise_amr[j] = True

        for i in range(0, len(steps)):
            if global_steps_to_evaluate_amr[i]:
                action_set_AMR_analysis = DynamicMeshRefinementAnalysis(
                    current_species_set,
                    min_particles_per_cell=int(
                        current_species_set.particle_model.particles_per_cell / 27
                    ),
                    max_particles_per_cell=current_species_set.particle_model.particles_per_cell,
                    min_h=current_species_set.particle_model.min_h,
                    max_h=current_species_set.particle_model.max_h,
                )
                action_set_AMR_analysis.descend_invocation_order = (
                    steps[i].lowest_descend_invocation_order() - 1
                )
                action_set_AMR_analysis.parallel = True
                steps[i].add_action_set(action_set_AMR_analysis)
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: add peano4.toolbox.particles.DynamicMeshRefinementAnalysis for species {} to step {}".format(
                            current_species_set.name, i
                        )
                    )

            if global_steps_to_update_parallel_state[i]:
                action_set_update_parallel_state = (
                    peano4.toolbox.particles.api.UpdateParallelState(
                        particle_set=current_species_set,
                    )
                )
                action_set_update_parallel_state.descend_invocation_order = (
                    steps[i].lowest_descend_invocation_order() - 1
                )
                action_set_update_parallel_state.parallel = True
                steps[i].add_action_set(action_set_update_parallel_state)
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: add peano4.toolbox.particles.UpdateParallelState for species {} to step {}".format(
                            current_species_set.name, i
                        )
                    )
#                assert (
#                    not current_species_set.generator.gather_particles
#                ), "the aggressive AMR is not compatible with particles that gather data in memory pools"
 # @todo Scattering is not in yet
                
                
            if global_steps_to_realise_amr[i]:
                action_set_dynamic_AMR = DynamicMeshRefinementTrigger()
                action_set_dynamic_AMR.descend_invocation_order = (
                    steps[i].highest_descend_invocation_order() + 1
                )
                action_set_dynamic_AMR.parallel = True
                steps[i].add_action_set(action_set_dynamic_AMR)
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: add swift2.actionsets.DynamicMeshRefinementTrigger for species {} to step {}".format(
                            current_species_set.name, i
                        )
                    )

            if global_steps_to_update_particle_grid_assocation[i]:
                if sort_with_lifts_and_drops:
                    action_set_update_particle_grid_association = peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(
                        current_species_set
                    )
                else:
                    action_set_update_particle_grid_association = peano4.toolbox.particles.api.UpdateParticleGridAssociation_BucketSort(
                        current_species_set
                    )
                action_set_update_particle_grid_association.descend_invocation_order = (
                    steps[i].lowest_descend_invocation_order() - 1
                )
                action_set_update_particle_grid_association.parallel = True
                steps[i].add_action_set(action_set_update_particle_grid_association)
                if verbose:
                    print(
                        "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption]: add peano4.toolbox.particles.api.UpdateParticleGridAssociation specialisation for species {} to step {}".format(
                            current_species_set.name, i
                        )
                    )
                    
                if current_species_set.generator.gather_particles:
                    action_set_gather = (
                        peano4.toolbox.particles.GatherParticlesInMemoryPool(
                            particle_set=current_species_set,
                        )
                    )
                    action_set_gather.descend_invocation_order = (
                        steps[i].lowest_descend_invocation_order() - 1
                    )
                    action_set_gather.parallel = True
                    
                    action_set_scatter = ScatterGlobalMemory(
                            particle_set=current_species_set,
                    )
                    action_set_scatter.descend_invocation_order = (
                        steps[i].lowest_descend_invocation_order() - 1
                    )
                    action_set_scatter.parallel = True
                    
                    #
                    # We have to add it once more, as a sieve or drop might trigger a "new" 
                    # neighbour exchange which then has to be taken into account on the 
                    # receiving side
                    #
                    steps[i].add_action_set(action_set_gather)
                    steps[i].add_action_set(action_set_scatter)
                    steps[(i + 1) % (len(steps))].add_action_set(action_set_gather)
                    if verbose:
                        print(
                            "GRAPH COMPILER DEBUG [add_dynamic_mesh_refinement_and_particle_resorting_minimise_resorting_and_add_gathers]: add peano4.toolbox.particles.GatherParticlesInMemoryPool for species {} to step {} and {}".format(
                                current_species_set.name, i, (i + 1) % (len(steps))
                            )
                        )


