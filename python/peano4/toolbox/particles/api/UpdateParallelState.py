# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

from peano4.toolbox.particles.api.AbstractUpdateParticleGridAssociation import (
    AbstractUpdateParticleGridAssociation,
)

import jinja2


class UpdateParallelState(ActionSet):
    """!

    Update the parallel state of particles and keep stats of them

    Particles within a parallel code hold a boolean flag ParallelState.
    It is this action set's responsibility to maintain
    this property. Users' codes may use the ParallelState, but as read-only
    only.

    We have two options per particle (held within ParallelState): local and virtual.
    In our data model, particles are either local or virtual throughout
    the mesh traversal, but they might change their position and
    consequently alter their state. Virtual particles can fly in through
    the boundary, while particles can leave the local domain (and hence become
    virtual) by moving through the boundary. Virtual particles (another
    word would be halo particles) are mere copies of particles somewhere
    else. You should not alter their value, and we can safely throw them
    away after a traversal, as the subsequent merger at the parallel
    boundary will re-introduce them. We can also keep them as long as
    we know that their position doesn't change. In this case, the merger
    will update the copies in-situ.

    To accommodate the fact that a particle moves and switches from local
    to virtual, we update its state in touchVertexFirstTime().

    When we hit a vertex and its particles for the last time, we can
    analyse the particles' parallel state. If the new state still is
    virtual, we throw the particle away. Otherwise we keep it and make
    the (real) parallel state equal to the new state. By comparing the
    parallel state which results from the previous grid sweeps and the new
    one, we can keep statistics how many particles are local, how many
    have flown into the domain, and how many have left the domain,


    ## Validity/correctness

    Particles may not change their position throughout the mesh traversal. They
    can change their position in touchVertexLastTime(). This effectively means
    they change x in-between two grid sweeps.

    It does not matter if you update particle positions in an action set prior
    or after this one is called. At the time you do something within
    touchVertexLastTime(), this action has already set the state markers
    accordingly and hence can make a decision.

    As a consequence, we can check that a particle can never switch from
    virtual to local here. Virtual particles should not move and therefore
    should not "enter" the domain. Local particles in contrast can move and
    hence leave the domain. That is, on a a neighbouring rank, a particle
    might be moved into "our" area. In this case, we eliminate our local
    virtual copy and then add the incoming particle as new local one.
    This merger however is the resonsibility of the actual particle set,
    and not of the actions here.


    ## Usage

    This operation should always been called after you've resorted the particles
    due to position changes or alterations of the search radii. Furthermore,
    the sketch above shows why it is important that virtual particles are
    always removed after a mesh traversal. This implies that all is good and
    fine as long as you use UpdateParallelState in each and every mesh
    traversal.

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
  _numberOfExpiredHaloParticles    = 0;
  _numberOfLeavingParticles        = 0;
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateNumberOfLocalAndExpiredParticles(
    _numberOfRemainingLocalParticles,
    _numberOfExpiredHaloParticles,
    _numberOfLeavingParticles
  );
"""
    )

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
for (auto& p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
  const bool particleWillBeLocal = toolbox::particles::particleAssignedToVertexWillBeLocal( p->getX(), marker );
  assertion2(
    not particleWillBeLocal
    or
    p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local,
    p->toString(),
    marker.toString()
  );
  // Has just left the domain. So we set it to virtual now. Next iteration,
  // the neighbour who has received this particle due to the movement will
  // send us an update, i.e. after this iteration we can throw it away.
  if (not particleWillBeLocal and p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local ) {
    p->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Virtual );
    _numberOfLeavingParticles++;
    toolbox::particles::assignmentchecks::removeParticleFromVertex( "{{PARTICLE}}", p->getX(), true, marker.x(), marker.h(), _treeNumber );
    toolbox::particles::assignmentchecks::assignParticleToVertex(   "{{PARTICLE}}", p->getX(), false, marker.x(), marker.h(), _treeNumber );
  }
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
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " is local and remains local" );
    _numberOfRemainingLocalParticles++;
    (*p)->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Local );
    p++;
  }
  else {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " is virtual. Remove local copy from vertex " << marker.toString() << " on tree " << _treeNumber );
    toolbox::particles::assignmentchecks::eraseParticle( "{{PARTICLE}}", (*p)->getX(), false, _treeNumber );
    _numberOfExpiredHaloParticles++;
    p = fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(p);
    #if !defined(Parallel) and !defined(SharedMemoryParallelisation)
    assertion(false);
    #endif
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
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Lock.h"
#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"

#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return """
  int  _treeNumber;

  int  _numberOfRemainingLocalParticles;
  int  _numberOfExpiredHaloParticles;
  int  _numberOfLeavingParticles;
"""

    def get_body_of_prepareTraversal(self):
        template = jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::clearParticleStateStatistics();
"""
        )
        return template.render(**self.d)
