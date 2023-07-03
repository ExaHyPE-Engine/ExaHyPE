# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation

import jinja2


class UpdateParallelState(ActionSet):
    """!

    Update the parallel state of particles and keep stats of local particles

    This operation should always been called after you've resorted the particles
    due to position changes or alterations of the search radii. The action set
    is the only place which heavily uses the particles' parallel states. These
    states are not to be used anywhere else in the code.


    ## Validity/correctness
    
    Particles may not change their position throughout the mesh traversal. They
    can change their position in touchVertexLastTime(). This effectively means
    they change x in-between two grid sweeps.
    
    It does not matter if you update particle positions in an action set prior
    or after this one is called. At the time you do something within 
    touchVertexLastTime(), this action has already set the state markers 
    accordingly and hence can make a decision.
    

    ## Statistics

    The mapping keeps some statistics on any state update. These statistics
    however are not shared via MPI. Instead, we hand them over to the
    particle set which then can consolidate the data among ranks and cores.


    ## State transitions

    We have two states per particle: local and virtual. In our data model,
    particles are either local or virtual throughout
    the mesh traversal, but they might change their position and
    consequently enter their state. Virtual particles can only fly in through
    the boundary, while particles can leave the local domain (and hence become
    virtual) by moving through the boundary.

    We recognise that we can throw away virtual particles after the sweep.
    We assume that they are temporary copies from another rank which will
    drop in again.

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

    """

    DefaultPriority = AbstractUpdateParticleGridAssociation.DefaultPriority + 1

    def __init__(self, particle_set):
        super(UpdateParallelState, self).__init__(
            priority=self.DefaultPriority, parallel=False
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
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was remote but now is local" );
    _numberOfIncomingLocalParticles++;
    (*p)->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Local );
    p++;
  }
  else if (
    (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
    and
    (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Virtual
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was local but now is remote. Remove local copy" );
    _numberOfLeavingLocalParticles++;
    p = fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(p);
  }
  else if (
    (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual
    and
    (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Virtual
  ) {
    logDebug( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " was virtual and remains virtual. Remove local copy" );
    _numberOfExpiredHaloParticles++;
    p = fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(p);
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
