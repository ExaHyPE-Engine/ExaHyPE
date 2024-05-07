# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation


class UpdateParticleGridAssociation_BucketSort(AbstractUpdateParticleGridAssociation):
    """!

    Associate particles to spacetree vertices

    Sort particles into the respective levels and cells. This mapping should
    be used every time you move particles around.


    ## Algorithm

    The present algorithm is a bucket sort. It consists of very few, simple
    steps:

    - If a particle has not moved or has moved but not approached another
      vertex, leave it where it is. We sort incrementally.
    - If a particle has moved and is not associated to its closest vertex
      anymore, add it to a global list.
    - If a particle has increased its cut-off radius and this cut-off
      radius does not fit into the present mesh resolution anymore, add it
      to a global list.
    - Exchange the global list between all ranks after each sweep.
    - If a particle from the global list fits into a cell, add it to the
      closest vertex.


    ## Dynamic adaptive mesh refinement

    If the grid is refined around a certain vertex, we usually have to drop
    particles associated to the coarser vertex - which is now a refined one -
    to the next finer level. However, such a drop would violate our bucketing
    principle. The other sorting algorithms are fine with drops, but we are
    not using them here.

    What we do instead is that we identify such particles and lift them into
    the sieve set. Which is counterintuitive as we lift particles which should
    be dropped. However, we know that the subsequent mesh sweep then will
    insert them into the right level. That is, we map a drop operation onto
    a lift-globally-sieve sequence.


    ## Contextualisation

    The algorithm describes an incremental bucket sort and naturally
    supports tunneling or massive changes of the cut-off. From an
    conceptional point of view, it is a stripped down version of the
    lift and drop variant without the lifts and the drops. After all,
    each lift becomes directly an additional to the global sieve list,
    and then we are done.

    So, in theory, we could basically take UpdateParticleGridAssociation_LiftDrop
    and remove the lifts and drops. However, this is slow.


    ## Impact on particle storage

    Whenever we remove a particle from the mesh and add it to the global sieve
    list or whenever we actually sieve a particle, i.e. add it to a vertex, we
    alter the respective vertex. However, we never ever alter the vertex lists
    from a coarser vertex. If this action set becomes a preamble to a gather,
    we hence always maintain continuous memory segments in line with
    @ref toolbox_particles_memorypool "Peano's memory pool discussion".


    ## Sieve set administration

    For bucket sort, each action set clones its own sieve set and then inserts
    all particles, i.e. both inside and virtual ones. This is important, as we
    want to have everything in the right place in one mesh sweep. We cannot
    just insert local particles (as in other action sets) and then rely on the
    boundary exchange to get the halos right. As each
    action set clones its own particle set, we can in return remove any
    particle successfully inserted.

    This means we have some high overhead initially to replicate all sieved
    particles. From thereon, we however run embarassingly parallel and bring
    the memory footprint successively down.

    """

    def __init__(self, particle_set, guard="true"):
        super(UpdateParticleGridAssociation_BucketSort, self).__init__(
            particle_set, guard
        )

    """
  
  Takes those particles which are not within h/2 reach of a vertex
  and lift those.
  
  """
    __Template_LiftParticles = jinja2.Template(
        """
  auto p = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (p!=fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
    toolbox::particles::ParticleReassociationInstruction instruction = toolbox::particles::liftParticleAssociatedWithVertex(**p, marker);
    switch ( instruction ) {
      case toolbox::particles::ParticleReassociationInstruction_Keep:
        if (toolbox::particles::particleWillBeDroppedFurther(**p,marker)) {
          _numberOfLiftsIntoSieveSet++;
          logDebug( "touchVertexLastTime(...)", "lift particle " << (*p)->toString() << " locally, as it has to be dropped to next level in next iteration" );
          toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles" );
          toolbox::particles::assignmentchecks::assignParticleToSieveSet( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, _spacetreeId, "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles" );
          p = fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(p);    
        }
        else {
          p++;
        }
        break;
      case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
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
        _numberOfLiftsIntoSieveSet++;
        logDebug( "touchVertexLastTime(...)", "lift particle " << (*p)->toString() << " globally. Previously assigned to " << marker.toString() );
        toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, marker.x(), marker.h(), _spacetreeId, "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles" );
        toolbox::particles::assignmentchecks::assignParticleToSieveSet( "{{PARTICLE}}", (*p)->getX(), (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local, _spacetreeId, "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles" );
        p = fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(p);    
        assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );    
        break;
      default:
        assertionMsg( false, "value not implemented" );
        break;
    }
  }
"""
    )

    """!
        
    The sieve here differs from the default one in the lift-drop variants. It
    inserts also remote particles, i.e. directly into the halo layer. It also
    does not eliminate particles from the sieve set.
        
    """
    _Template_Sieve = jinja2.Template(
        """
  if ( _sieveParticleSet.hasParticlesToBeSievedIntoVertices() ) {
    auto sievedParticles = _sieveParticleSet.getParticlesToBeSievedIntoVertex( 
      marker,
      true,   // removeReturnedParticlesFromSet,
      false,  // onlyReturnParticlesThatWillBeLocal
      false   // don't lock particle set, as we work with our thread-local copy
    );

    for (auto& sievedParticle: sievedParticles) {
      _numberOfDropsFromSieveSet++;      
      fineGridVertex{{PARTICLES_CONTAINER}}.mergeWithParticle( 
        new globaldata::{{PARTICLE}}(*sievedParticle), 
        marker, 
        _spacetreeId 
      );
    }

    assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isValid(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        """!

        Core actions triggered when we run through the mesh

        See class description. If you compare this function to
        UpdateParticleGridAssociation_LiftDrop.get_body_of_operation(), we see
        that we use our modified lift routine when particles have moved and in
        return can omit the dropping. That is, the cousin's realisation is

        ~~~~~~~~~~~~~~~~~~~~~~~~~~
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result =  self._Template_Drop.render(**self.d)
            result += self._Template_Sieve.render(**self.d)
        ~~~~~~~~~~~~~~~~~~~~~~~~~~

        while we only do the sieve here.

        """
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._Template_BeginTraversal.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._Template_Sieve.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_LiftParticles.render(**self.d)
        if operation_name == ActionSet.OPERATION_DESTROY_PERSISTENT_VERTEX:
            result = self._Template_LiftAllParticles.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._Template_EndTraversal.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_constructor_body(self):
        return (
            super(UpdateParticleGridAssociation_BucketSort, self).get_constructor_body()
            + jinja2.Template(
                """
            _sieveParticleSet = vertexdata::{{PARTICLES_CONTAINER}}::cloneParticlesToBeSieved();
          """
            ).render(**self.d)
        )

    def get_destructor_body(self):
        return (
            super(UpdateParticleGridAssociation_BucketSort, self).get_destructor_body()
            + jinja2.Template(
                """
            _sieveParticleSet.deleteParticles();
          """
            ).render(**self.d)
        )

    def get_attributes(self):
        return (
            super(UpdateParticleGridAssociation_BucketSort, self).get_attributes()
            + jinja2.Template(
                """
  toolbox::particles::SieveParticles< globaldata::{{PARTICLE}} >  _sieveParticleSet;
"""
            ).render(**self.d)
        )
