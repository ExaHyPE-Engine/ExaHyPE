# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation


class UpdateParticleGridAssociation_LiftDrop(AbstractUpdateParticleGridAssociation):
    """!

    Associate particles to spacetree vertices

    Sort particles into the respective levels and cells. This mapping should
    be used every time you move particles around.


    ## Algorithm

    The re-association has different ingredients. We have two implementations
    currently available. The present version does not touch any particle within
    the cell events. It realises the following steps/checks:

    - If I touch a vertex for the last time, I check if its particles are
      within h/2 Manhattan distance. If not, I lift them to the next coarser
      level.
    - Drop particles down within the tree hierarchy such that they reside on
      an as fine level as possible. This happens in touchVertexFirstTime(), i.e.
      in-between two grid traversals, particles are distributed over all
      resolutions, but we move them down in the hierarchy as kind of a preamble
      of the subsequent grid sweep.

    This variant is algorithmically simpler than other ones, but it moves particles
    around more often: If a particle moves closer to another vertex than the
    one it is currently associated, we do not directly reassign it to a new
    vertex. Instead, we move it one level up within the tree. Just before the
    subsequent grid sweep, I sieve the particles all down again. So the move
    from one vertex to the next is realised via a move through the next coarser
    level. The first variant does not move particles up and down that often, but
    it checks particles more often, as each particle is checked from up to 2^d
    adjacent cells. So it is not clear which variant is superior.


    ## Tunneling and horizontal data movements between trees

    All cores run through their trees. On the finest mesh level, we have a
    non-overlapping domain decomposition. Each cell has a unique owner tree.
    On the coarser resolutions, the association is not unique anymore. We
    make it unique, i.e. each cell on any level has a unique owner. The owners
    (aka threads or ranks) run through their trees concurrently and exchange
    their boundary data after the traversal. If particles are to be lifted, this
    causes issues, as we cannot exchange them between two levels after the
    traversal. We could, but then we can't lift them any further within the
    first traversal, and we also cannot drop them into another rank at the
    same time.

    Therefore, I employ a mixture of the pidt technique from the paper and the
    sieve approach also discussed there. Usually, I do all lifts and drops
    right throughout the mesh traversal. If I'd lift into a cell/level which
    is owned by another tree, I don't lift but instead dump the particle into
    a rank-global list. These lists are then exchanged after each traversal
    sweep. Drops now can either happen from the next coarser level or this
    global list. I call the latter a sieve.

    Due to this global list approach, I can support tunneling, i.e. particles
    racing through multiple cells in one time step, and all of this works with
    MPI, too.


    ## Statistics

    The mapping keeps some statistics on any state update. These statistics
    however are not shared via MPI or synchronised between different threads.
    It is the job of the ParticleSet to ensure that we have a proper global
    bookkeeping.


    ## Where and how to use

    Within your algorithm step, this update action set should always be the
    first or one of the first action sets before you add any other action set.
    If you add it first, it ensures that the drop of particles is one of the
    first things that happen, before any user code is triggered. As Peano
    automatically inverts the action set order throughout the backtracking,
    adding UpdateParticleGridAssociation as first action set ensures that the
    lifts are basically the last thing you do. This is particular imporant if
    your user code alters particle positions in touchVertexLastTime().

    Your main code has to call finishedTraversal() on the tracer set after each
    sweep on each rank. This important to roll those particles over that have
    been sieved or have been lifted.

    You will always needs to instances of this action set in a row, as the first
    one might lift particles and put them into a sieve list. After that, the
    subsequent grid sweep will drop the particles and sieve them in again. As
    long as particles do not move, this variant becomes idempotent after two
    mesh sweeps.


    ## Hanging vertices

    Hanging vertices do never hold information and therefore are never assigned
    any particles.


    ## Interpolay with particle storage

    This sorting scheme should not be used if you use a particle memory pool,
    i.e. if you try to store particles continuously - unless you insert
    explicit gather/resort steps. Read through the @ref toolbox_particles_memorypool "memory implications for any particle sorting".


    """

    def __init__(self, particle_set, guard="true"):
        super(UpdateParticleGridAssociation_LiftDrop, self).__init__(
            particle_set, guard
        )

    _Template_Sieve = jinja2.Template(
        """
  if ( vertexdata::{{PARTICLES_CONTAINER}}::hasParticlesToBeSievedIntoVertices() ) {
    auto sievedParticles = vertexdata::{{PARTICLES_CONTAINER}}::getParticlesToBeSievedIntoVertex(
      marker
    );

    for (auto& newParticle: sievedParticles) {
      _numberOfDropsFromSieveSet++;
      fineGridVertex{{PARTICLES_CONTAINER}}.mergeWithParticle( newParticle, marker, _spacetreeId );
    }

    assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
  }
"""
    )

    """!

  Takes those particles which are not within h/2 reach of a vertex and lift those.

  This code snippet is used with touchVertexLastTime(). Therefore, the marker
  is a vertex marker.

  """
    __Template_LiftOrReassignParticles = jinja2.Template(
        """
  auto p = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (p!=fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
    toolbox::particles::ParticleReassociationInstruction instruction = toolbox::particles::liftParticleAssociatedWithVertex(**p, marker);
    switch ( instruction ) {
      case toolbox::particles::ParticleReassociationInstruction_Keep:
        p++;
        break;
      case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
        _numberOfLiftsIntoSieveSet++;
        logDebug( "touchVertexLastTime(...)", "have to lift particle " << (*p)->toString() << " on tree " << _spacetreeId << " globally. Previously assigned to " << marker.toString() );

        toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles" );
        toolbox::particles::assignmentchecks::assignParticleToSieveSet( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, _spacetreeId, "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles" );

        p = fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(p);
        assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
        break;
      case 0:
      case 1:
      case 2:
      case 3:
      #if Dimensions==3
      case 4:
      case 5:
      case 6:
      case 7:
      #endif
        _numberOfLifts ++;
        logDebug( "touchVertexLastTime(...)", "have to lift particle " << (*p)->toString() << " to next coarser vertex " << instruction << ". Previously assigned to " << marker.toString() << ". Now lifted to " << coarseGridVertices{{PARTICLES_CONTAINER}}( instruction ).toString() );
        (*p)->setCellH(3.0 * marker.h());
        toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles" );
        toolbox::particles::assignmentchecks::assignParticleToVertex(
          "{{PARTICLE}}",
          (*p)->getX(),
          (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local,
          peano4::datamanagement::reconstructXOfParentVertex(marker, instruction),
          3.0*marker.h(), _spacetreeId, "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles"
        );
        p = coarseGridVertices{{PARTICLES_CONTAINER}}( instruction ).grabParticle( p, fineGridVertex{{PARTICLES_CONTAINER}} );
        assertion2( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),                    fineGridVertex{{PARTICLES_CONTAINER}}.toString(), coarseGridVertices{{PARTICLES_CONTAINER}}( instruction ).toString() );
        assertion2( coarseGridVertices{{PARTICLES_CONTAINER}}( instruction ).isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString(), coarseGridVertices{{PARTICLES_CONTAINER}}( instruction ).toString() );
        break;
      default:
        assertionMsg( false, "value not implemented" );
        break;
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        """!

        Core algorithm. See class description.

        """
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._Template_BeginTraversal.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._Template_Drop.render(**self.d)
            result += self._Template_Sieve.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_LiftOrReassignParticles.render(**self.d)
        if operation_name == ActionSet.OPERATION_DESTROY_PERSISTENT_VERTEX:
            result = self._Template_LiftAllParticles.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._Template_EndTraversal.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
