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


    ## Merges along domain boundary
    
    We would like all state updates to be localised within this action set.
    However, we cannot do everything here: We also have to update the 
    parallel state when we merge along the domain boundary. This happens
    in the particle set's merge(). The documentation around merge() is 
    summarise here, as we have multiple particle set implementations, but
    all of them are realised through Jinja templates. So it is better to
    have stuff in one place only.
    
    The merge in principle is trivial: If a neighbour sends you in a 
    virtual particle, don't do anything with it. Just ignore it. If it
    is local at the neighbour, we add it to our local vertex.
    
    @image html UpdateParallelState_merge.png
    
    The sketch above illustrates why it is important to neglect virtual
    particles sent in: In the sketch, the left subdomain sends its 
    particle (green) over to the right, where it is marked as virtual 
    (red). The right one now sends this particle back (dark green) and 
    we would hold redundant information.
    
    The sketch also illustrates why we throw away virtual particles after
    the mesh sweep: If the particle moves on the owning rank - and it can
    only move there - the neighbour would have two virtual copies (as it 
    cannot match old one to new one). So we better throw away all old stuff
    by default.
    
    With that data flow in mind, particles crossing domain boundaries are
    automatically given the correct state. We wouldn't have to implement
    and local/non-local logic in the merge(). This could all be done in 
    this action set. However, we still need some logic here. 

    The reason is simple: A merge happens in each and every mesh sweep, 
    even if particles do not move. When a particle flies into a domain 
    (it doesn't have to fly, most of the time it has been there anyway)
    and is a local one at the neighbour, we cannot imply toggle it to 
    virtual here, as it might be owned redundantly if it sits directly
    on the domain boundaries. We have to rerun the "is local
    here" analysis after each merge.

    
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
  _numberOfParticlesThatHaveLeftTheirDomain        = 0;
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateNumberOfLocalAndExpiredParticles(
    _numberOfRemainingLocalParticles,
    _numberOfExpiredHaloParticles,
    _numberOfParticlesThatHaveLeftTheirDomain
  );
"""
    )

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
for (auto& p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
  const bool particleWillBeLocal = toolbox::particles::particleAssignedToVertexWillBeLocal( p->getX(), marker );

  // Have left the domain in the previous mesh sweep. So we set it to virtual 
  // now. After this iteration, the particle will be thrown away. This can only
  // happen if the previous mesh sweep has altered the position, i.e. while we 
  // set the particle to virtual here, some other tree will just receive this 
  // one and set it local there. In most cases, this will be a replace of an 
  // existing (virtual) particle copy.
  if (not particleWillBeLocal and p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local ) {
    p->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Virtual );
    logDebug( "touchVertexLastTime(...)", "particle has left domain=" << p->toString() << " (attached to vertex " << marker.toString() << ", will be deleted after this sweep but should be inflying on other tree)" );
    _numberOfParticlesThatHaveLeftTheirDomain++;
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
    p++;
  }
  else {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " is virtual. Remove local copy from vertex " << marker.toString() << " on tree " << _treeNumber );
    toolbox::particles::assignmentchecks::detachParticleFromVertex( "{{PARTICLE}}", (*p)->getX(), false, marker.x(), marker.h(), _treeNumber, "UpdateParallelState::__Template_TouchVertexLastTime" );
    toolbox::particles::assignmentchecks::eraseParticle( "{{PARTICLE}}", (*p)->getX(), false, _treeNumber, "UpdateParallelState::__Template_TouchVertexLastTime" );
    #if !defined(Parallel)
    assertion2(
      ::peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()>1,
      (*p)->toString(),
      marker.toString()
    );
    #endif
    _numberOfExpiredHaloParticles++;
    p = fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(p);
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

#include "peano4/parallel/SpacetreeSet.h"

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
  int  _numberOfParticlesThatHaveLeftTheirDomain;
"""

    def get_body_of_prepareTraversal(self):
        template = jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::clearParticleStateStatistics();
"""
        )
        return template.render(**self.d)
