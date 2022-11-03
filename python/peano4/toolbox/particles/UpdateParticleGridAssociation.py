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
   It is the job of the ParticleSet to ensure that we have a proper global
   bookkeeping.
   
   
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
   
   Your main code has to call finishedTraversal() on the tracer set after each
   sweep on each rank. This important to roll those particles over that have 
   been sieved or have been lifted.
   
      
   ## Parameters
   
   lift_always: Boolean
     See discussion of the two algorithms that I realise here. By default, 
     I do not sort within cells but move up and down within the grid 
     hierarchy.
     
   warn_if_particles_tunnel: Boolean
   
  """
  def __init__(self,
               particle_set, 
               guard="true", 
               lift_always=True ):
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                      = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]           = particle_set.name
    self.d[ "GUARD" ]                         = guard
    self._lift_always                         = lift_always


  __Template_BeginTraversal = jinja2.Template("""
  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts  = 0;
  _numberOfDrops  = 0;
  _numberOfSieves = 0;
  
  _sieveParticles = vertexdata::{{PARTICLES_CONTAINER}}::getSievedParticlesForNextGridTraversal();
""" )


  __Template_EndTraversal = jinja2.Template("""
  vertexdata::{{PARTICLES_CONTAINER}}::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfSieves
  );

  _sieveParticles.deleteParticles();
""" )


  """
  
  @see toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment()
  
  """
  __Template_ReassignAndLiftWithinCell = jinja2.Template("""
  for (int i=0; i<TwoPowerD; i++) {
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = fineGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=fineGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      toolbox::particles::ParticleReassociationInstruction instruction = 
        toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
          p, marker, i
        );
      switch ( instruction ) {
        case toolbox::particles::ParticleReassociationInstruction_Keep:
          p++;
          break;
        case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
          // @todo Debug
          logInfo( "touchVertexLastTime(...)", "particle " << (*p)->toString() << " has to be sieved globally. Current assocation=" << marker.toString() );
          fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(*p);    
          p = fineGridVertex{{PARTICLES_CONTAINER}}.erase(p);
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
          fineGridVertices{{PARTICLES_CONTAINER}}(instruction).push_back(*p);
          p = fineGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
          _numberOfParticleReassignmentsOnCurrentLevel++;
          break;
        case 0+TwoPowerD:
        case 1+TwoPowerD:
        case 2+TwoPowerD:
        case 3+TwoPowerD:
        #if Dimensions==3
        case 4+TwoPowerD:
        case 5+TwoPowerD:
        case 6+TwoPowerD:
        case 7+TwoPowerD:
        #endif
          coarseGridVertices{{PARTICLES_CONTAINER}}(instruction-TwoPowerD).push_back(*p);
          p = fineGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
          _numberOfLifts ++;
          break;
        default:
          assertionMsg( false, "value not implemented" );
          break;
      }      
    }
  }
  
""")


  __Template_ValidateParticleLevelAssociation = jinja2.Template("""
  for (auto p: fineGridVertex{{PARTICLES_CONTAINER}} ) {
    assertion3(
      marker.isContainedInAdjacentCells( p->getX(), 0.5*(1.0+SortingTolerance) ),
      marker.x(),
      marker.toString(),
      p->toString()
    );
  }
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
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      if ( 
        tarch::la::min( marker.h() ) > 2.0 * (*p)->getCutOffRadius()
        and
        // You may never ever use some tolerance here: If you only drop a 
        // particle into the slightly wrong vertex, its status likely won't
        // get updated to local. The particle will hence be labelled as purely
        // virtual and eventually disappear.
        marker.isContainedInAdjacentCells( (*p)->getX(), 0.5 * (1.0-SortingTolerance) )
      ) {
        fineGridVertex{{PARTICLES_CONTAINER}}.push_back( *p );
        p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }  
""")


  __Template_Sieve = jinja2.Template("""
  if ( not _sieveParticles.empty() ) {
    vertexdata::{{PARTICLES_CONTAINER}} sievedParticles = _sieveParticles.getParticlesToBeSievedIntoVertex(
        marker,
        1.0+SortingTolerance
      );
      
    _numberOfSieves += sievedParticles.size();

    fineGridVertex{{PARTICLES_CONTAINER}}.insert(
      fineGridVertex{{PARTICLES_CONTAINER}}.end(),
      sievedParticles.begin(),
      sievedParticles.end()
    );
    
    fineGridVertex{{PARTICLES_CONTAINER}}.eliminateSpatialDuplicates(false, 1.0+SortingTolerance);
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
    fineGridVertex{{PARTICLES_CONTAINER}}.particlesCanNotBeLiftedLocally();
  }
""")  


  """
  
  Takes those particles which are not within h/2 reach of a vertex
  and lift those.
  
  """
  __Template_LiftParticlesFurtherAway = jinja2.Template("""
  auto p = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (p!=fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
    toolbox::particles::ParticleReassociationInstruction instruction = toolbox::particles::liftParticleAssociatedWithVertex(**p, marker);
    switch ( instruction ) {
      case toolbox::particles::ParticleReassociationInstruction_Keep:
        p++;
        break;
      case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
        // @todo Debug (auch an der anderen Stelle)
        logInfo( "touchVertexLastTime(...)", "have to lift particle " << (*p)->toString() << " globally. Previously assigned to " << marker.toString() );
        fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(*p);    
        p = fineGridVertex{{PARTICLES_CONTAINER}}.erase(p);
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
        coarseGridVertices{{PARTICLES_CONTAINER}}( instruction ).push_back( *p );
        _numberOfLifts ++;
        p = fineGridVertex{{PARTICLES_CONTAINER}}.erase(p);
        break;
      default:
        assertionMsg( false, "value not implemented" );
        break;
    }
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
      pass
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result  = self.__Template_Drop.render(**self.d) 
      result += self.__Template_Sieve.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      if self._lift_always:
        result = self.__Template_LiftParticlesFurtherAway.render(**self.d)
      else:    
        result = self.__Template_ValidateParticleLevelAssociation.render(**self.d)
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
#include "toolbox/particles/MultiscaleTransitions.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
""" )
    return result.render(**self.d)             


  def get_attributes(self):
    return jinja2.Template( """
  int  _treeNumber;
  int  _numberOfParticleReassignmentsOnCurrentLevel;
  int  _numberOfLifts;
  int  _numberOfDrops;
  int  _numberOfSieves;
  
  vertexdata::{{PARTICLES_CONTAINER}}  _sieveParticles;
""").render(**self.d)


  def get_static_initialisations(self,full_qualified_classname):
    return jinja2.Template("""
namespace {
  constexpr double SortingTolerance = 1e-8;
}
""").render(**self.d)    
