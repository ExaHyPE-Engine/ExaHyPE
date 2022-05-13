# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class UpdateParticleGridAssociation(ActionSet):
  """
   Associate particles to spacetree vertices
   
   Sort particles into the respective levels and cells. This mapping should 
   be used every time you move particles around. Notably, you should invoke
   it prior to the first algorithms on the particles. 
   
   The re-association has different ingredients. A lot of them are discussed
   in the LaTeX documentation:
   
   - Look at all the particles within a cell and check whether they are 
     still assigned to the closest of the @f$ 2^d @f$ vertices of this 
     cell. If not, reassign them.
   - Identify particles that have moved by more than one mesh width since the 
     last update. If a particle travels fast, lift it to the next level.
   - Drop particles down within the tree hierarchy such that they reside on 
     an as fine level as possible.
     
   
   ## Statistics 
     
   The mapping keeps some statistics on any state update. These statistics
   however are not shared via MPI. 
   
   ## Where and how to use
   
   Idempotent. Plugs into touchLast, d.h. Partikel muessen in Zelle sich bewegen
   
       
   ## Parameters
   
   eliminate_spatial_duplicates: String
     A C++ boolen expression. Whenever it holds, the code runs over the 
     particles at the end of the sweep and simply kicks out duplicates.
   
  """
  def __init__(self,particle_set, guard="true", eliminate_spatial_duplicates="false"):
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                      = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]           = particle_set.name
    self.d[ "GUARD" ]                         = guard
    self.d[ "ELIMINATE_DUPLICATES_GUARD" ]    = eliminate_spatial_duplicates


  __Template_BeginTraversal = jinja2.Template("""
  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts = 0;
  _numberOfDrops = 0;
  _numberOfParticleAccesses = 0;
  _numberOfEliminations = 0;
""" )


  __Template_EndTraversal = jinja2.Template("""
  logInfo( 
    "endTraversal()",
    "tree " <<  _treeNumber <<
    ": reassignments=" << _numberOfParticleReassignmentsOnCurrentLevel << 
    ", lift=" << _numberOfLifts << 
    ", drops=" << _numberOfDrops << 
    {% if ELIMINATE_DUPLICATES_GUARD %}
    ", eliminated (duplicate) particles=" << _numberOfEliminations <<
    {% endif %}
    ", accesses=" << _numberOfParticleAccesses 
  );
""" )


  """
  
  This is where we actually run over the particles and check if they are 
  still associated to the right vertex. As we want to accommodate 
  position changes, we do this as last step in the routine.
  
  """
  __Template_TouchCellLastTime = jinja2.Template("""
if ( {{GUARD}} ) {
  // move means particles that are actually to be associated with a new nearest vertex  
  vertexdata::{{PARTICLES_CONTAINER}} particlesThatMoveOnSameLevel[TwoPowerD];
  // particles that 'jumped' aka moved by more than half a mesh width
  vertexdata::{{PARTICLES_CONTAINER}} liftParticles[TwoPowerD];

  // run over all adjacent vertices, and find the bad guys, i.e. those 
  // particles that are not associated to the correct nearest vertex
  // anymore
  for (int i=0; i<TwoPowerD; i++) {
    std::bitset<Dimensions> currentAssociation;
    std::bitset<Dimensions> destAssociation;
    bool                    remainsOnSameLevel;
    
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = fineGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=fineGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      _numberOfParticleAccesses++;
      
      // study only particles which are contained within the current cell
      if ( marker.isContained((*p)->getX()) ) {
        std::bitset<Dimensions>  currentAssociation = i;
        bool                     remainsOnSameLevel = true;
        // marker.x() gives me the centre of the cell
        for (int d=0; d<Dimensions; d++) {
          destAssociation[d]  = tarch::la::greaterEquals( (*p)->getX(d), marker.x()(d) );
          remainsOnSameLevel &= tarch::la::smallerEquals( std::abs((*p)->getX(d) - marker.x()(d)), marker.h()(d));
        }
      
        if (remainsOnSameLevel and currentAssociation==destAssociation) {
          p++;
        }
        else if (remainsOnSameLevel) {
          logDebug( "touchCellLastTime(...)", "reassign particle " << (*p)->toString() << " in tree " << _treeNumber << " within cell " << marker.toString() << " which had been associated with vertex " << i << " but belongs to " << destAssociation );
          particlesThatMoveOnSameLevel[ destAssociation.to_ulong() ].push_back(*p);
          p = fineGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
          _numberOfParticleReassignmentsOnCurrentLevel++;
        }
        else {
          logDebug( "touchCellLastTime(...)", "lift particle " << (*p)->toString() << " in tree " << _treeNumber << " from cell " << marker.toString() << " as difference is " << ((*p)->getX() - marker.x()) );
          liftParticles[ destAssociation.to_ulong() ].push_back(*p);
          p = fineGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
          _numberOfLifts++;
        }
      }
      else {
        p++;
      }
    }
  }
  
  // we have now extracted all particles which are not associated with their 
  // nearest vertex anymore and we have put them into the two local lists.
  // Finally, we thus have to run through the two local lists particlesThatMoveOnSameLevel
  // and liftParticles and have to append those into the respective vertices.
  for (int i=0; i<TwoPowerD; i++) {
    fineGridVertices{{PARTICLES_CONTAINER}}(i).insert(   fineGridVertices{{PARTICLES_CONTAINER}}(i).end(),   particlesThatMoveOnSameLevel[ i ].begin(), particlesThatMoveOnSameLevel[ i ].end() ); 
    coarseGridVertices{{PARTICLES_CONTAINER}}(i).insert( coarseGridVertices{{PARTICLES_CONTAINER}}(i).end(), liftParticles[ i ].begin(),                liftParticles[ i ].end() ); 
  }
}
""")


  __Template_ValidateParticleLevelAssociation = jinja2.Template("""
  for (auto p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
    assertion3(
      marker.isContainedInAdjacentCells( p->getX(), 0.5 ),
      marker.x(),
      marker.toString(),
      p->toString()
    );
  }
""")


  __Template_EliminateDuplicateParticles = jinja2.Template("""
  if ( ({{GUARD}}) and ({{ELIMINATE_DUPLICATES_GUARD}}) ) {
    _numberOfEliminations += fineGridVertex{{PARTICLES_CONTAINER}}.eliminateSpatialDuplicates(false);
  }
""")
    

  """
  
   Happens when we touch a vertex for the first time. The only thing this routine
   does is to drop particles within the tree, i.e. move them to finer levels. 
   No parallelisation is taken into account. 
   
  """
  __Template_Drop = jinja2.Template("""
  const double CellToCutOffRadiusRatio = 0.5 * 0.9;
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      bool dropIntoThisVertex = tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius();
      for (int d=0; d<Dimensions; d++) {
        dropIntoThisVertex &= (*p)->getX()(d) > marker.x()(d) - marker.h()(d)/2.0;
        dropIntoThisVertex &= (*p)->getX()(d) < marker.x()(d) + marker.h()(d)/2.0;
      }
      
      if (dropIntoThisVertex) {
        fineGridVertex{{PARTICLES_CONTAINER}}.push_back( *p );
        p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }
""")


  """
  
  Explicit lift of particles to a coarser level. Usually, lifts are
  realised within touchCellLastTime aka the template 
  __Template_TouchCellLastTime. If a vertex is however destroyed or
  is hanging, i.e. not persistent in-between two mesh traversals, 
  then we have to lift the particles explicitly.
  
  """
  __Template_Lift = jinja2.Template("""
  std::bitset<Dimensions> target;
  for (int d=0; d<Dimensions; d++) {
    target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
  }
  coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).insert( coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).begin(), fineGridVertex{{PARTICLES_CONTAINER}}.begin(), fineGridVertex{{PARTICLES_CONTAINER}}.end() );
""")  


  def get_constructor_body(self):
    return """
  _treeNumber  = treeNumber;
"""
    

  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = self.__Template_BeginTraversal.render(**self.d) 
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_VERTEX:
      result = self.__Template_Drop.render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_Drop.render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result  = self.__Template_ValidateParticleLevelAssociation.render(**self.d)
      result += self.__Template_EliminateDuplicateParticles.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d) 
    if operation_name==ActionSet.OPERATION_DESTROY_HANGING_VERTEX:
      result = self.__Template_Lift.render(**self.d) 
    if operation_name==ActionSet.OPERATION_DESTROY_PERSISTENT_VERTEX:
      result = self.__Template_Lift.render(**self.d) 
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
  int  _numberOfParticleReassignmentsOnCurrentLevel;
  int  _numberOfLifts;
  int  _numberOfDrops;
  int  _numberOfParticleAccesses;
  int  _numberOfEliminations;
"""


