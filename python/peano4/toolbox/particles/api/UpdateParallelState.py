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


    ## Validity/correctness

    Particles may not change their position throughout the mesh traversal. They
    can change their position in touchVertexLastTime(). This effectively means
    they change x in-between two grid sweeps. Consequently, their state might
    be wrong in-between two sweeps.

    This routine has to be injected ***after*** additional particles are
    added to the mesh. That can happen throughout boundary merges or due to
    resorts. This action set has to be used after each mesh sweep that
    alters particle positions.

    The action set throws away halo particles in touchVertexFirstTime().
    Therefore, if you don't use it after each and every mesh sweep, you
    might get redundant data at the boundary (due to repeated particle
    inflow) or outdated data. Both is automatically avoided in the boundary
    merges, which eliminate replica.


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
  _numberOfIncomingParticles       = 0;
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateNumberOfLocalAndExpiredParticles(
    _numberOfRemainingLocalParticles,
    _numberOfExpiredHaloParticles,
    _numberOfLeavingParticles,
    _numberOfIncomingParticles
  );
"""
    )

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
for (auto& p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
  const bool particleWillBeLocal = toolbox::particles::particleAssignedToVertexWillBeLocal( p->getX(), marker );

  if (particleWillBeLocal and p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual ) {
    p->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Local );
    _numberOfIncomingParticles++;
    toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", p->getX(), false, marker.x(), marker.h(), _treeNumber, "UpdateParallelState::__Template_TouchVertexFirstTime" );
    toolbox::particles::assignmentchecks::assignParticleToVertex(   "{{PARTICLE}}", p->getX(), true,  marker.x(), marker.h(), _treeNumber, "UpdateParallelState::__Template_TouchVertexFirstTime" );
  }

  // Has just left the domain. So we set it to virtual now. Next iteration,
  // the neighbour who has received this particle due to the movement will
  // send us an update, i.e. after this iteration we can throw it away.
  if (not particleWillBeLocal and p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local ) {
    p->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Virtual );
    _numberOfLeavingParticles++;
    toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", p->getX(), true, marker.x(), marker.h(), _treeNumber, "UpdateParallelState::__Template_TouchVertexFirstTime" );
    toolbox::particles::assignmentchecks::assignParticleToVertex(   "{{PARTICLE}}", p->getX(), false, marker.x(), marker.h(), _treeNumber, "UpdateParallelState::__Template_TouchVertexFirstTime" );
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
    toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), false, marker.x(), marker.h(), _treeNumber, "UpdateParallelState::__Template_TouchVertexLastTime" );
    toolbox::particles::assignmentchecks::eraseParticle( "{{PARTICLE}}", (*p)->getX(), false, _treeNumber, "UpdateParallelState::__Template_TouchVertexLastTime" );
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
  int  _numberOfIncomingParticles;
"""

    def get_body_of_prepareTraversal(self):
        template = jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::clearParticleStateStatistics();
"""
        )
        return template.render(**self.d)
