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


   ## Algorithm 
      
   The re-association has different ingredients. We have two implementations
   currently available. 
   
   In the fist version, we try to minimise the particle re-associations. This
   algorithm realises the key checks within the cells:
   
   - Look at all the particles within a cell and check whether they are 
     still assigned to the closest of the @f$ 2^d @f$ vertices of this 
     cell. If not, reassign them.
   - Identify particles that have moved by more than one mesh width since the 
     last update. If a particle travels fast, lift it to the next level. This
     check happens recursively whenever we touch a cell for the last time.
   - Drop particles down within the tree hierarchy such that they reside on 
     an as fine level as possible. This happens in touchVertexFirstTime(), i.e.
     in-between two grid traversals, particles are distributed over all 
     resolutions, but we move them down in the hierarchy as kind of a preamble
     of the subsequent grid sweep.
     
   I also offer a second version which does not do anything within the cell
   events:
   
   - If I touch a vertex for the last time, I check if its particles are 
     within h/2 Manhattan distance. If not, I lift them to the next coarser
     level.
   - Drop particles down within the tree hierarchy such that they reside on 
     an as fine level as possible. This happens in touchVertexFirstTime(), i.e.
     in-between two grid traversals, particles are distributed over all 
     resolutions, but we move them down in the hierarchy as kind of a preamble
     of the subsequent grid sweep.
   
   This second variant is algorithmically simpler, but it moves particles 
   around more often: If a particle moves closer to another vertex than the 
   one it is currently associated, we do not directly reassign it to a new
   vertex. Instead, we move it one level up within the tree. Just before the 
   subsequent grid sweep, I sieve the particles all down again. So the move
   from one vertex to the next is realised via a move through the next coarser
   level. The first variant does not move particles up and down that often, but
   it checks particles more often, as each particle is checked from up to 2^d 
   adjacent cells. So it is not clear which variant is superior. 
   
   There is one functional difference however: If you use the cell-based 
   variant, the variant assumes that particles do not move after 
   touchVertexFirstTime(). So you can alter them in touchVertexFirstTime() 
   and then the code will sort them properly in this sweep, but if you change
   their position again in touchCellLastTime() or touchVertexLastTime(), then
   you will have an unsorted state by the end of the traversal. 
   
   
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
   
   
   ## Where and how to use
   
   The action set is idempotent, i.e. you can merge it into your 
   algorithmic step multiple times. Only the first one merged in however will
   alter the particle positions, so it is not clever -from a performance point
   of view - to host multiple instances of this action set.
   
   Within your algorithm step, this update action set should always be the 
   first or one of the first action sets before you add any other action set.
   If you add it first, it ensures that the drop of particles is one of the 
   first things that happen, before any user code is triggered. As Peano 
   automatically inverts the action set order throughout the backtracking,
   adding UpdateParticleGridAssociation as first action set ensures that the
   lifts are basically the last thing you do. This is particular imporant if
   your user code alters particle positions in touchVertexLastTime().
   
      
   ## Parameters
   
   eliminate_spatial_duplicates: Boolean
     If it holds, the code runs over the 
     particles at the end of the sweep and simply kicks out duplicates. This
     happens on a per-vertex basis.
     
   lift_always: Boolean
     See discussion of the two algorithms that I realise here. By default, 
     I do not sort within cells but move up and down within the grid 
     hierarchy.
   
  """
  def __init__(self,
               particle_set, 
               guard="true", 
               eliminate_spatial_duplicates=False,
               lift_always=True):
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                      = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]           = particle_set.name
    self.d[ "GUARD" ]                         = guard
    self.d[ "ELIMINATE_DUPLICATES_GUARD" ]    = eliminate_spatial_duplicates
    self._eliminate_spatial_duplicates        = eliminate_spatial_duplicates
    self.d[ "LIFT_ALWAYS" ]                   = lift_always
    self._lift_always                         = lift_always


  __Template_BeginTraversal = jinja2.Template("""
  _globalSieveParticleList.clear();
  _globalSieveParticleList.insert( _globalSieveParticleList.end(), _globalMovingParticleList.begin(), _globalMovingParticleList.end() );
  _globalSieveParticleListIsEmpty = _globalMovingParticleList.empty();
  _globalMovingParticleList.clear();

  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts = 0;
  _numberOfDrops = 0;
  _numberOfEliminations = 0;
  _numberOfAssignmentsToGlobalSieve = 0;
""" )


  __Template_EndTraversal = jinja2.Template("""
  logInfo( 
    "endTraversal()",
    "tree " <<  _treeNumber <<
    {% if not LIFT_ALWAYS %}
    ": reassignments=" << _numberOfParticleReassignmentsOnCurrentLevel << 
    {% endif %}
    {% if ELIMINATE_DUPLICATES_GUARD %}
    ", eliminated (duplicate) particles=" << _numberOfEliminations <<
    {% endif %}
    ", lifts=" << _numberOfLifts << 
    ", drops=" << _numberOfDrops <<
    ", assignments-to-global-sieve=" << _numberOfAssignmentsToGlobalSieve
  );
""" )





  """
  
  This is where we actually run over the particles and check if they are 
  still associated to the right vertex. As we want to accommodate 
  position changes, we do this as last step in the routine.
  
  """
  __Template_ReassignAndLiftWithinCell = jinja2.Template("""
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
  }

  if (marker.isCellLocal()) {
    for (int i=0; i<TwoPowerD; i++) {
      coarseGridVertices{{PARTICLES_CONTAINER}}(i).insert( coarseGridVertices{{PARTICLES_CONTAINER}}(i).end(), liftParticles[ i ].begin(), liftParticles[ i ].end() ); 
      _numberOfLifts += liftParticles[ i ].size();
    }
  }
  else {
    tarch::multicore::Lock lock( _globalMovingParticleSemaphore );
    _globalMovingParticleList.insert( coarseGridVertices{{PARTICLES_CONTAINER}}(i).end(), liftParticles[ i ].begin(), liftParticles[ i ].end() );
    _numberOfAssignmentsToGlobalSieve += liftParticles[ i ].size();
  }
""")


  __Template_ValidateParticleLevelAssociation = jinja2.Template("""
  for (auto p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
    assertion3(
      marker.isContainedInAdjacentCells( p->getX(), 0.5*SortingTolerance ),
      marker.x(),
      marker.toString(),
      p->toString()
    );
  }
""")


  """
  
  Plugs into touchVertexLastTime() for example.
  
  """
  __Template_EliminateDuplicateParticles = jinja2.Template("""
  _numberOfEliminations += fineGridVertex{{PARTICLES_CONTAINER}}.eliminateSpatialDuplicates(false);
""")
    

  """
  
   Happens when we touch a vertex for the first time. The only thing this routine
   does is to drop particles within the tree, i.e. move them to finer levels. 
   No parallelisation is taken into account. 
   
  """
  __Template_Drop = jinja2.Template("""
  // I can always drop. Even if some of the coarse vertices are outside,
  // they will be properly initialised and thus host an empty particle 
  // list.
  for (int i=0; i<TwoPowerD; i++) {
    // Define inside for loop, as sieve will define it as well
    const double CellToCutOffRadiusRatio = 0.5 * (1.0-SortingTolerance);
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      if ( 
        tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius()
        and
        marker.isContainedInAdjacentCells( (*p)->getX(), 0.5*SortingTolerance )
      ) {
        logDebug( "touchVertexLastTime(...)", "drop particle " << (*p)->toString() << " into vertex " << marker.toString() );
        fineGridVertex{{PARTICLES_CONTAINER}}.push_back( *p );
        p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }

    for (auto& p: coarseGridVertices{{PARTICLES_CONTAINER}}(i)) {
      bool cutOffRadiusSmallEnough = tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * p->getCutOffRadius();
      logDebug( "touchVertexFirstTime(...)", "have not been able to drop particle " << p->toString() << " from coarse vertex " << coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString() << " into vertex " << fineGridVertex{{PARTICLES_CONTAINER}}.toString() << ". cut-off small enough=" << cutOffRadiusSmallEnough );
    }
  }  
""")


  __Template_Sieve = jinja2.Template("""  
  if (not _globalSieveParticleListIsEmpty) {
    const double CellToCutOffRadiusRatio = 0.5 * (1.0-SortingTolerance);
    tarch::multicore::Lock lock( _globalMovingParticleSemaphore );
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = _globalSieveParticleList.begin(); 
    while (p!=_globalSieveParticleList.end()) {
      if ( 
        tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius()
        and
        marker.isContainedInAdjacentCells( (*p)->getX(), 0.5*SortingTolerance )
      ) {
        logDebug( "touchVertexLastTime(...)", "drop particle " << (*p)->toString() << " from global sieve list into vertex " << marker.toString() );
        fineGridVertex{{PARTICLES_CONTAINER}}.push_back( *p );
        p = _globalSieveParticleList.erase(p);
        _numberOfDrops++;
      }
      else {
        p++;
      }
    }
    _globalSieveParticleListIsEmpty  = _globalSieveParticleList.empty();
  }
  
  
  
  
  
  // I can always drop. Even if some of the coarse vertices are outside,
  // they will be properly initialised and thus host an empty particle 
  // list.
  const double CellToCutOffRadiusRatio = 0.5 * 0.9;
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      if ( 
        tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * (*p)->getCutOffRadius()
        and
        marker.isContainedInAdjacentCells( (*p)->getX(), 0.5*SortingTolerance )
      ) {
        logDebug( "touchVertexLastTime(...)", "drop particle " << (*p)->toString() << " into vertex " << marker.toString() );
        fineGridVertex{{PARTICLES_CONTAINER}}.push_back( *p );
        p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }

    for (auto& p: coarseGridVertices{{PARTICLES_CONTAINER}}(i)) {
      bool cutOffRadiusSmallEnough = tarch::la::min( marker.h() ) > CellToCutOffRadiusRatio * p->getCutOffRadius();
      logDebug( "touchVertexFirstTime(...)", "have not been able to drop particle " << p->toString() << " from coarse vertex " << coarseGridVertices{{PARTICLES_CONTAINER}}(i).toString() << " into vertex " << fineGridVertex{{PARTICLES_CONTAINER}}.toString() << ". cut-off small enough=" << cutOffRadiusSmallEnough );
    }
  }  
""")


  """
  
  Explicit lift of particles to a coarser level. Usually, lifts are
  realised within touchCellLastTime aka the template 
  __Template_ReassignAndLiftWithinCell. If a vertex is however destroyed or
  is hanging, i.e. not persistent in-between two mesh traversals, 
  then we have to lift the particles explicitly.
  
  """
  __Template_Lift = jinja2.Template("""
  if ( marker.isParentCellLocal() ) {
    std::bitset<Dimensions> target;
    for (int d=0; d<Dimensions; d++) {
      target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
    }
    _numberOfLifts+=fineGridVertex{{PARTICLES_CONTAINER}}.size();
    coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).insert( coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).end(), fineGridVertex{{PARTICLES_CONTAINER}}.begin(), fineGridVertex{{PARTICLES_CONTAINER}}.end() );
  }
  else {
    tarch::multicore::Lock lock( _globalMovingParticleSemaphore );
    _globalMovingParticleList.insert( _globalMovingParticleList.end(), fineGridVertex{{PARTICLES_CONTAINER}}.begin(), fineGridVertex{{PARTICLES_CONTAINER}}.end() );
    _numberOfAssignmentsToGlobalSieve += fineGridVertex{{PARTICLES_CONTAINER}}.size();
  }
""")  


  """
  
  Takes those particles which are not within h/2 reach of a vertex
  and lift those.
  
  """
  __Template_LiftParticlesFurtherAway = jinja2.Template("""
  vertexdata::{{PARTICLES_CONTAINER}} liftParticles;
  
  auto p = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (p!=fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
  if ( not marker.isContainedInAdjacentCells((*p)->getX(),0.5) ) {
    logDebug( "touchVertexLastTime(...)", "lift particle " << (*p)->toString() );
      liftParticles.push_back(*p);
      p = fineGridVertex{{PARTICLES_CONTAINER}}.erase(p);
    }
    else {
      p++;
    }
  }
  
  // If the parent cell is local, then we can safely move the particles
  // up the tree, as we know that all 2^d coarse grid vertices are 
  // adjacent to the local domain. If the parent is not local, we have
  // to assume that we just did hit a vertical tree cut. In this case, 
  // we are on the safe side if we just add the particles to the global
  // particle list.
  if ( not liftParticles.empty() and marker.isParentCellLocal() ) {
    std::bitset<Dimensions> target;
    for (int d=0; d<Dimensions; d++) {
      target[d] = marker.getRelativePositionWithinFatherCell()(d)>=2;
    }
    if (not liftParticles.empty()) {
      logDebug( "touchVertexLastTime(...)", "add " << liftParticles.size() << " to the parent vertex " << coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).toString() << " at " << target );
    }
    coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).insert( 
      coarseGridVertices{{PARTICLES_CONTAINER}}( target.to_ulong() ).end(),
      liftParticles.begin(),
      liftParticles.end()
    );
    _numberOfLifts += liftParticles.size();
  } 
  else if (not liftParticles.empty()) {
    tarch::multicore::Lock lock( _globalMovingParticleSemaphore );
    logDebug( "touchVertexLastTime(...)", "add " << liftParticles.size() << " to the global sorting list" );
    _globalMovingParticleList.insert( _globalMovingParticleList.end(), liftParticles.begin(), liftParticles.end() );
    _numberOfAssignmentsToGlobalSieve += liftParticles.size();
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
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_VERTEX:
      result  = self.__Template_Drop.render(**self.d) 
      result += self.__Template_Sieve.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result  = self.__Template_Drop.render(**self.d) 
      result += self.__Template_Sieve.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      if self._lift_always:
        result  = self.__Template_LiftParticlesFurtherAway.render(**self.d)
      else:    
        result  = self.__Template_ValidateParticleLevelAssociation.render(**self.d)
      if self._eliminate_spatial_duplicates:        
        result += self.__Template_EliminateDuplicateParticles.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      if not self._lift_always:
        result = self.__Template_ReassignAndLiftWithinCell.render(**self.d) 
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
    return jinja2.Template( """
  int  _treeNumber;
  int  _numberOfParticleReassignmentsOnCurrentLevel;
  int  _numberOfLifts;
  int  _numberOfAssignmentsToGlobalSieve;
  int  _numberOfDrops;
  int  _numberOfEliminations;

  /**
   * Protect access of _globalMovingParticleList on the node.
   */
  tarch::multicore::BooleanSemaphore   _globalMovingParticleSemaphore;
  
  typedef vertexdata::{{PARTICLES_CONTAINER}} GlobalMovingParticlesList;
  
  /**
   * For vertical particle movements (between resolutions) which we cannot realise
   * within one tree.
   */
  static GlobalMovingParticlesList  _globalMovingParticleList;
  static GlobalMovingParticlesList  _globalSieveParticleList;
  
  /**
   * Set if global sieve list is empty at begin of traversal.
   */
  static bool                       _globalSieveParticleListIsEmpty;

""").render(**self.d)


  def get_static_initialisations(self,full_qualified_classname):
    return jinja2.Template( 
full_qualified_classname + """::GlobalMovingParticlesList  """ + full_qualified_classname + """::_globalMovingParticleList;
""" + full_qualified_classname + """::GlobalMovingParticlesList  """ + full_qualified_classname + """::_globalSieveParticleList;
bool  """ + full_qualified_classname + """::_globalSieveParticleListIsEmpty;

namespace {
  constexpr double SortingTolerance = 1.01;
}
""").render(**self.d)    
