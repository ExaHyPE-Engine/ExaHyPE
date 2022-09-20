# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class UpdateParallelState(ActionSet):
  """
   Update the parallel state of particles
     
   
   ## Statistics 
     
   The mapping keeps some statistics on any state update. These statistics
   however are not shared via MPI. Instead, we hand them over to the 
   particle set which then can consolidate the data among ranks and cores.
    
   
   ## State transitions
   
   We have two states per particle: local and virtual. In our data model, 
   particles are either local or virtual throughout 
   the mesh traversal, but they might change their position and 
   consequently enter the regime local-to-virtual. As we solely update local 
   particles, it can never happen that you have virtual-to-local. Such a 
   transition would happen on another tree (where it is a local-to-virtual
   transition from this tree's point of view) and then would be sent back 
   as update.
   
   At the begin of a traversal, i.e. when we 
   touch a particle for the first time, we set the new state to virtual. 
   This happens via the template __Template_InitialiseParallelStatus. 
   
   When we leave a cell, i.e. after all cell-local operations have finished,
   we run over all particles associated to its @f$ 2^d @f$ 
   vertices and look if a particle's position x falls into a cell. If so,
   we set the particle's state to local.
   
   If we touch a vertex for the last time, we run over its particles. If 
   a particle has been local and still is local, all is fine. If a particle
   is virtual and has been local, we know it has left the domain. We keep
   it (as we have to tell the others that this particle might have moved 
   over), but we flag it as virtual from hereon. If a particle has been
   virtual and still is virtual, we delete it. These are particles that 
   we have received from other trees to run our calculations, so they are 
   remote copies. We should not keep such data and notably not distribute
   it further.   
   
   ## Usage
   
   This action set is not idempotent, i.e. you have to merge it at most once
   into your algorithmic step.
   
   Please add this action set as soon as possible to your code. This ensures
   that the touchCellLastTime() is called as late as possible and likely 
   takes any particle movement into account. In most codes, this action set
   will be the second one used, while UpdateParticleGridAssociation is the 
   first one.
   
   If a particle changes its position in touchVertexLastTime() and leaves
   the current domain, we will not be able to spot this. We will need one
   more grid sweep to get all the states consistent again. If you work 
   with a parallel computer, I therefore recommend that you never check 
   the particle state re its local or virtual flag. I recommend that you 
   simply assume that the fact that a particle is within the local set is 
   totally sufficient to update it.
   
  """
  def __init__(self,particle_set):
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                      = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]           = particle_set.name


  __Template_BeginTraversal = jinja2.Template("""
  _numberOfLocalParticles = 0;
  _numberOfVirtualParticles = 0;
  _numberOfLeavingParticles = 0;
""" )


  __Template_EndTraversal = jinja2.Template("""
  vertexdata::{{PARTICLES_CONTAINER}}::updateParticleCountStatistics(
    _numberOfLocalParticles,
    _numberOfVirtualParticles
  );
  
  logInfo( 
    "endTraversal()",
    "tree=" <<  _treeNumber <<
    ", local particles=" << _numberOfLocalParticles << 
    ", virtual particles=" << _numberOfVirtualParticles << 
    ", leaving particles=" << _numberOfLeavingParticles
  );
""" )


  """
  
  Flag those particles which are definitely inside. As we are really
  aggressive in throwing particles away, there's no guard here.
  
  """
  __Template_TouchCell = jinja2.Template("""
// run over all adjacent vertices
for (int i=0; i<TwoPowerD; i++) {
  for (auto* p: fineGridVertices{{PARTICLES_CONTAINER}}(i) ) {
    if ( marker.isContained(p->getX()) ) {
      p->setNewParallelState( globaldata::{{PARTICLE}}::NewParallelState::Local );
    }
  }
}
""")


  
  __Template_InitialiseParallelStatus = jinja2.Template("""
  for (auto* p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
    p->setNewParallelState( globaldata::{{PARTICLE}}::NewParallelState::Virtual );
  }
""")


  """
  
  This postamble of the grid traversal plugs into touchVertexLastTime().
   
  """
  __Template_DetermineNewParallelStatus = jinja2.Template("""
    auto p = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
    while (p!=fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
      if ( 
        (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
        and 
        (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Virtual
      ) {
        logInfo( "touchVertexLastTime()", "local status from previous sweep not confirmed on tree " << _treeNumber << ". Particle " << (*p)->toString() << " has left domain. Keep for the time being as a virtual particle" );
        (*p)->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Virtual );
        _numberOfLeavingParticles++;
        p++;
      }
      else if ( 
        (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Local
        and 
        (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Local
      ) {
        logDebug( "touchVertexLastTime()", "local status confirmed on tree " << _treeNumber << ", i.e. particle " << (*p)->toString() << " remains local" );
        _numberOfLocalParticles++;
        p++;
      }
      else if ( 
        (*p)->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual
        and 
        (*p)->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Virtual
      ) {
        logDebug( "touchVertexLastTime()", "virtual particle, i.e. from other tree. Delete local copy " << (*p)->toString() << " on tree " << _treeNumber );
        _numberOfVirtualParticles++;
        delete *p;
        p = fineGridVertex{{PARTICLES_CONTAINER}}.erase(p);
      }
      else {
        logDebug( 
          "touchVertexLastTime()", 
          "new particle " << (*p)->toString() << " had been originally labelled as virtual but now is confirmed as local on tree " << _treeNumber <<
          ". This can happen throughout particle insertion or due to lifts and sieves"
        );
        (*p)->setParallelState( globaldata::{{PARTICLE}}::ParallelState::Local );
        _numberOfLocalParticles++;
        p++;
      }
    }
    for (auto* p: fineGridVertex{{PARTICLES_CONTAINER}}) {
      assertion3(
        not (p->getParallelState()==globaldata::{{PARTICLE}}::ParallelState::Virtual and p->getNewParallelState()==globaldata::{{PARTICLE}}::NewParallelState::Local),
        p->toString(),
        marker.toString(),
        _treeNumber
      );
    }
""")


  def get_constructor_body(self):
    return """
  _treeNumber  = treeNumber;
"""
    

  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = self.__Template_BeginTraversal.render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_InitialiseParallelStatus.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = self.__Template_DetermineNewParallelStatus.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCell.render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCell.render(**self.d) 
    if operation_name==ActionSet.OPERATION_END_TRAVERSAL:
      result = self.__Template_EndTraversal.render(**self.d) 
    return result


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False
  
  
  def get_includes(self):
    result = jinja2.Template( """
#include "tarch/multicore/Lock.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
""" )
    return result.render(**self.d)             


  def get_attributes(self):
    return """
  int  _treeNumber;
  int  _numberOfLocalParticles;
  int  _numberOfVirtualParticles;
  int  _numberOfLeavingParticles;
"""


