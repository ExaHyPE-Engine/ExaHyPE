# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

from peano4.toolbox.particles.api.AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation

import jinja2


class UpdateParallelState(ActionSet):
    """!

    Update the parallel state of particles and keep stats of local particles

    Particles within a parallel code hold two boolean flags: The ParallelState
    and the NewParallelState. These flags are defined within
    peano4.toolbox.Particle. It is this action set's responsibility to maintain
    this state. That means

    1. codes may use the ParallelState, but for read-only purposes only.
    2. codes may never use the NewParallelState.

    We have two states per particle (held within ParallelState): local and virtual. In our data model,
    particles are either local or virtual throughout
    the mesh traversal, but they might change their position and
    consequently alter their state. Virtual particles can only fly in through
    the boundary, while particles can leave the local domain (and hence become
    virtual) by moving through the boundary. Virtual particles (another
    word would be halo particles) are mere copies of particles somewhere
    else. You should not alter their value, and we can safely throw them
    away after a traversal, as the subsequent merger at the parallel
    boundary will re-introduce them. We can also keep them as long as
    we know that their position doesn't change. In this case, the merger
    will update the copies in-situ.

    The new state analysis is very simple:
    - We clear the flag in touchVertexFirstTime(), i.e. all the particles
      associated to a vertex are initially set to virtual. We actually
      clear the new flag which is a working field, whereas the real parallel
      state is always the valid one.
    - Within a cell, we mark all local particles within this cell as local.
    - When we hit a vertex and its particles for the last time, we can
      analyse the particles' parallel state. If the new state still is
      virtual, we throw the particle away. Otherwise we keep it and make
      the (real) parallel state equal to the new state. By comparing the
      parallel state which results from the previous grid sweeps and the new
      one, we can keep statistics how many particles are local, how many
      have flown into the domain, and how many have left the domain,


    ## Scenarios

    I explain a number of use cases below. From these use cases, it becomes
    immediately clear why and how the action set has to be used. Let two
    ranks be adjacent to each other (top and down). In Peano, the ranks do
    not really have overlapping cells, but it helps to think of overlapping
    cells in the context of Peano's @ref page_toolbox_particles_mesh_storage "dual tree storage paradigm".

    @image html UpdateParallelState.png

    If the left rank owns a particle (green), this particle will be send over
    to the right rank after the first traversal. Prior to the second grid
    sweep, the green particle is received on the right rank, and this local
    copy is labelled as virtual in its parallel state (light red).

    As ranks always send out their whole vertex set after a traversal, the
    left rank will re-receive this virtual copy in the next mesh traversal.
    Here, it can safely ignore it. It is received from the right neighbour
    where it already has been labelled as virtual, i.e. the right neighbour
    has shared its halo data, too. We don't have to work it into our own
    data structure.

    In the third sweep, the right rank will also receive yet another copy of
    green (maybe with updated values). So it can either throw away the halo
    data after each mesh sweep or update its data structures. Just don't keep
    the old halo copies.

    It also could happen in the third sweep that the particle has crossed
    the domain boundary (third row, right) and thus effectively changed
    ownership. In this case, it once again helps to throw away all local
    halo data, receive the neighbours data, and re-assess the data. The
    neighbour should, by the time it sends out data, still label its particle
    as local, but then update the x. So, in a sense, the combination of
    position and state is inconsistent. We take the particle, add it locally
    as halo data and then reset it once we recognise on the right rank that
    we've actually now owning these data.


    ## Validity/correctness

    Particles may not change their position throughout the mesh traversal. They
    can change their position in touchVertexLastTime(). This effectively means
    they change x in-between two grid sweeps.

    It does not matter if you update particle positions in an action set prior
    or after this one is called. At the time you do something within
    touchVertexLastTime(), this action has already set the state markers
    accordingly and hence can make a decision.


    ## Usage

    This operation should always been called after you've resorted the particles
    due to position changes or alterations of the search radii. Furthermore,
    the sketch above shows why it is important that virtual particles are
    always removed after a mesh traversal. This implies that all is good and
    fine as long as you use UpdateParallelState in each and every mesh
    traversal.

    However, it is sometimes desireable not to use the action set in each and
    every mesh traversal. It changes the particle-mesh association as it
    eliminates particles. Therefore, it does not work well with coalesced
    memory arrangements or really any storage scheme which tries to work with
    continuous AoS for the particles.

    If you want to use the particle resorting only every now and then, you have
    to take into account that you might have duplicate (invalid) halo data in
    the one traversal directly after you have changed the particle positions.
    As long as you insert a dummy iteration after the change which finalises
    the sorting, these invalid data are not an issue. They are also no issues
    if a re-arrangement is followed by a mesh sweep which does not use halo
    data. However, if you use halo data, then you have to take into account
    that the halo data might be wrong for one iteration (see bottom sketch above).

    See add_dynamic_mesh_refinement_and_particle_resorting_aggressive_mesh_adoption()
    in swift2.graphcompiler.AMRLogic for a realisation of the above rationale.
    In contrast, map_particle_steps_onto_separate_mesh_traversals_insert_dummy_sweeps_for_coalesced_memory_access()
    uses dummy sweeps.


    ## Statistics

    The mapping keeps some statistics on any state update. These statistics
    however are not shared via MPI. Instead, we hand them over to the
    particle set which then can consolidate the data among ranks and cores.


    """

    DefaultDescendInvocationOrder = (
        AbstractUpdateParticleGridAssociation.DefaultDescendInvocationOrder + 1
    )

    def __init__(
        self,
        particle_set,
    ):
        super(UpdateParallelState, self).__init__(
            descend_invocation_order=self.DefaultDescendInvocationOrder, parallel=False
        )
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name

    __Template_BeginTraversal = jinja2.Template(
        """
  _numberOfRemainingLocalParticles = 0;
  _numberOfIncomingLocalParticles  = 0;
  _numberOfLeavingLocalParticles   = 0;
  _numberOfExpiredHaloParticles    = 0;
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateNumberOfLocalParticleStatistics(
    _numberOfRemainingLocalParticles,
    _numberOfIncomingLocalParticles,
    _numberOfLeavingLocalParticles,
    _numberOfExpiredHaloParticles
  );
"""
    )

    """

  Flag those particles which are definitely inside. As we are really
  aggressive in throwing particles away, there's no guard here.

  """
    __Template_TouchCell = jinja2.Template(
        """
// run over all adjacent vertices
for (int i=0; i<TwoPowerD; i++) {
  for (auto* p: fineGridVertices{{PARTICLES_CONTAINER}}(i) ) {
    if (
      marker.isContained(p->getX(), toolbox::particles::relativeSpatialSortingTolerance(marker) * tarch::la::NUMERICAL_ZERO_DIFFERENCE )
    ) {
      logDebug( "touchCellFirstTime(...)", "label particle " << p->toString() << " as local on tree " << _treeNumber );
      p->setNewParallelState( globaldata::{{PARTICLE}}::NewParallelState::Local );
    }
  }
}
"""
    )

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
// run over all adjacent vertices
for (auto p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
  // data consistency check
  assertion3(
    p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
    or 
    p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual,
    p->toString(),
    fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
    marker.toString()
  );
  p->setNewParallelState( globaldata::{{PARTICLE}}::NewParallelState::Virtual );
}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
// run over all adjacent vertices
auto p = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
while ( p!=fineGridVertex{{PARTICLES_CONTAINER}}.end() ) {
  if (
    (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
    and
    (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Local
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " is local and remains local" );
    _numberOfRemainingLocalParticles++;
    (*p)->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Local );
    p++;
  }
  else if (
    (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual
    and
    (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Local
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was remote but now is local. Label copy at vertex " << marker.toString() << " on tree " << _treeNumber << " as local" );
    _numberOfIncomingLocalParticles++;
    (*p)->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Local );
    p++;
    #if !defined(Parallel) and !defined(SharedMemoryParallelism)
    assertion(false);
    #endif
  }
  else if (
    (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
    and
    (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Virtual
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was local but now is remote. Remove local copy" );
    _numberOfLeavingLocalParticles++;
    p = fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(p);
    #if !defined(Parallel) and !defined(SharedMemoryParallelism)
    assertion(false);
    #endif
  }
  else if (
    (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual
    and
    (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Virtual
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was virtual and remains virtual. Remove local copy from vertex " << marker.toString() << " on tree " << _treeNumber );
    _numberOfExpiredHaloParticles++;
    p = fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(p);
    #if !defined(Parallel) and !defined(SharedMemoryParallelism)
    assertion(false);
    #endif
  }
  else {
    assertion3( false, (*p)->toString(), fineGridVertex{{PARTICLES_CONTAINER}}.toString(), marker.toString() );
  }
}
"""
    )

    def get_constructor_body(self):
        return """
  _treeNumber  = treeNumber;
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self.__Template_BeginTraversal.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_TouchVertexFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCell.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_TouchVertexLastTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "toolbox/particles/MultiscaleTransitions.h"

#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return """
  int  _treeNumber;

  int  _numberOfRemainingLocalParticles;
  int  _numberOfIncomingLocalParticles;
  int  _numberOfLeavingLocalParticles;
  int  _numberOfExpiredHaloParticles;
"""

    def get_body_of_prepareTraversal(self):
        template = jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::clearParticleStateStatistics();
"""
        )
        return template.render(**self.d)
