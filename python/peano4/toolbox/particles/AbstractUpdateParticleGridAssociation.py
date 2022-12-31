# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


   


class AbstractUpdateParticleGridAssociation(ActionSet):
  """
   Associate particles to spacetree vertices

   Use one of the subclasses which realise various sorting algorithms.
   
  """
  def __init__(self,
               particle_set, 
               guard="true" ):
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                      = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]           = particle_set.name
    self.d[ "GUARD" ]                         = guard


  _Template_BeginTraversal = jinja2.Template("""
  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts  = 0;
  _numberOfDrops  = 0;
  _numberOfSieves = 0;
  
  _sieveParticles = vertexdata::{{PARTICLES_CONTAINER}}::getSievedParticlesForNextGridTraversal();
""" )


  _Template_EndTraversal = jinja2.Template("""
  vertexdata::{{PARTICLES_CONTAINER}}::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfSieves
  );

  _sieveParticles.deleteParticles();
""" )


  _Template_ValidateParticleLevelAssociation = jinja2.Template("""
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
  _Template_Drop = jinja2.Template("""
  for (int i=0; i<TwoPowerD; i++) {
    // Define inside for loop, as sieve will define it as well
    vertexdata::{{PARTICLES_CONTAINER}}::iterator p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).begin(); 
    while ( p!=coarseGridVertices{{PARTICLES_CONTAINER}}(i).end() ) {
      if ( ::toolbox::particles::dropParticle(**p,marker) ) {
        fineGridVertex{{PARTICLES_CONTAINER}}.push_back( *p );
        p = coarseGridVertices{{PARTICLES_CONTAINER}}(i).erase(p);
        _numberOfDrops++;
      }
      else p++;
    }
  }  
""")


  _Template_Sieve = jinja2.Template("""
  if ( not _sieveParticles.empty() ) {
    vertexdata::{{PARTICLES_CONTAINER}} sievedParticles = _sieveParticles.getParticlesToBeSievedIntoVertex( marker );
      
    _numberOfSieves += sievedParticles.size();

    fineGridVertex{{PARTICLES_CONTAINER}}.insert(
      fineGridVertex{{PARTICLES_CONTAINER}}.end(),
      sievedParticles.begin(),
      sievedParticles.end()
    );

    fineGridVertex{{PARTICLES_CONTAINER}}.eliminateSpatialDuplicates();
  }
""")  
  
  
  

  """
  
  Explicit lift of particles to a coarser level. Usually, lifts are
  realised within touchCellLastTime aka the template 
  __Template_ReassignAndLiftWithinCell. If a vertex is however destroyed or
  is hanging, i.e. not persistent in-between two mesh traversals, 
  then we have to lift the particles explicitly.
  
  """
  _Template_LiftAllParticles = jinja2.Template("""
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


  def get_constructor_body(self):
    return """
  _treeNumber  = treeNumber;
"""
    

  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


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

