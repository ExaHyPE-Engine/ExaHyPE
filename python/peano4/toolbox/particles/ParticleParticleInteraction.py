# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class ParticleParticleInteraction(ActionSet):
  def __init__(self, particle_set, cell_compute_kernel, touch_vertex_first_time_compute_kernel=None, touch_vertex_last_time_compute_kernel=None, additional_includes="", guard="true", active_particle_set=None ):
    """

    This code snippet creates a tree walker, i.e. an action set that runs
    through the underlying spacetree top down. It builds up sets recursively.

    image:: dependency_sets.png

    Per tree node there are two different sets. We explain it by means of
    the light blue cell in the cartoon. Cells of this level hold particles
    whose cut-off radius is stricly smaller or equal to the mesh size.

    The set of local particles of a cell are all of these particles whose
    centre is contained within the cell plus a halo of h/2. This is the
    dotted area around the cell. That is, the local set holds also particles
    that are not centred within the cell.

    We need the local set to cover an area that is bigger than the actual
    cell as we want to capture all particles of this level that might
    theoretically interact with the particles located within the cell. If
    you want to update particles of a cell, you should thus only update
    particles whose centre is within the cell: Particles belong the the
    local sets of up to 2^d surrounding cells, so otherwise might be updated
    multiple times.

    The yellow particle on the fine grid in the sketch above is so far away
    from the cell that it does not belong into the set of local particles of
    the highlighted cell.

    The set of active particles is the local set plus the active set from
    the father cell in the spacetree. This is a recursive cell. So the set
    of active particles holds all local particles plus all the particles of
    coarser levels which might overlap with a local particle whose centre is
    within the current cell.

    The dark blue particles in the sketch is an example of such a particle.
    The yellow particle on the coarser mesh level in constrast is not
    contained within the active set of the blue cell, as it is simply too
    far away - even on the coarser mesh where we again work with a "halo"
    around the coarse cell of h_{coarse}/2.

    Besides the cell kernel which is there to realise particle-to-particle
    interactions, we also have a vertex kernel which we call whenever a
    vertex is loaded for the first time. That is, you can assume that the
    vertex kernel has been launched for all 2^d vertices of a cell before
    its cell kernel is triggered. The vertex kernel is also passed on the
    active set (from the coarser level). Its local set is all the particles
    whose centre lies within the square/cube around the vertex with mesh size
    h. So this area goes h/2 along each each coordinate axis into the
    neighbouring cells.

    Please consult the guidebook for further information.


    particle_set: ParticleSet

    cell_compute_kernel: String holding C++ code
      This C++ code can access three different types of variables: There's
      a list of particles called activeParticles, there's a list of particles
      called localParticles, and there's the cell marker. See the guidebook
      for further info.
      
    active_particle_set: ParticleSet or None
      You can compare different particles species with this argument. It
      allows you to make the active particles stem from another species than
      the local ones that you actually update. Pick None if both sets are of
      the same type.
      
    touch_vertex_first_time_compute_kernel: String or None
      Can be empty, but if you wanna move particles, then a minimal example
      string passed equals
        
          localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
   
      i.e. you will have a variable localParticle available in this kernel
      and it is a pointer.

    """
    self._particle_set = particle_set
    self.d = {}
    self.d[ "LOCAL_PARTICLE" ]                    = particle_set.particle_model.name
    self.d[ "LOCAL_PARTICLES_CONTAINER" ]         = particle_set.name
    if active_particle_set==None:
      self.d[ "ACTIVE_PARTICLE" ]                 = particle_set.particle_model.name
      self.d[ "ACTIVE_PARTICLES_CONTAINER" ]      = particle_set.name
    else:
      self.d[ "ACTIVE_PARTICLE" ]                 = active_particle_set.particle_model.name
      self.d[ "ACTIVE_PARTICLES_CONTAINER" ]      = active_particle_set.name
    self.d[ "CELL_COMPUTE_KERNEL" ]               = cell_compute_kernel
    self.d[ "TOUCH_VERTEX_FIRST_COMPUTE_KERNEL" ] = touch_vertex_first_time_compute_kernel
    self.d[ "TOUCH_VERTEX_LAST_COMPUTE_KERNEL" ]  = touch_vertex_last_time_compute_kernel
    self.d[ "ADDITIONAL_INCLUDES" ]               = additional_includes
    self.d[ "GUARD" ]                             = guard
    

  __Template_TouchVertexFirstTime = jinja2.Template("""
  auto& localParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  for (auto& localParticle: localParticles ) {
    {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
  }
""")


  __Template_TouchVertexLastTime = jinja2.Template("""
  auto& localParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  for (auto& localParticle: localParticles ) {
    {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
  }
""")


  __Template_TouchCellFirstTime = jinja2.Template("""
  if ( {{GUARD}} ) {
    std::list< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
    _numberOfActiveParticlesAdded.push_back(0);
    for (int i=0; i<TwoPowerD; i++) {
      for (auto& p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
        _activeParticles.push_front( p );
        _numberOfActiveParticlesAdded.back()++;
      }
      for (auto& p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
        bool append = marker.isContained( p->getX() );
        if (append) {
          localParticles.push_front( p );
        }
      }
    }

    std::list< globaldata::{{ACTIVE_PARTICLE}}* >&  activeParticles = _activeParticles;
    for (auto& localParticle: localParticles )
    for (auto& activeParticle: activeParticles ) {
      {{CELL_COMPUTE_KERNEL}}
    }
  }
""")


  __Template_TouchCellLastTime = jinja2.Template("""
  for (int i=0; i<_numberOfActiveParticlesAdded.back(); i++) {
    _activeParticles.pop_back();
  }
  _numberOfActiveParticlesAdded.pop_back();
""")


  __Template_EndTraversal = jinja2.Template("""
  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    #if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
    #  result = self.__Template_BeginTraversal.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME and self.d[ "TOUCH_VERTEX_FIRST_COMPUTE_KERNEL" ]!=None:
      result = self.__Template_TouchVertexFirstTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME and self.d[ "TOUCH_VERTEX_LAST_COMPUTE_KERNEL" ]!=None:
      result = self.__Template_TouchVertexLastTime.render(**self.d)
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
#include "vertexdata/{{LOCAL_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{LOCAL_PARTICLE}}.h"
#include "vertexdata/{{ACTIVE_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{ACTIVE_PARTICLE}}.h"

{{ADDITIONAL_INCLUDES}}

#include <list>
#include <vector>
""" )
    return result.render(**self.d)


  def get_attributes(self):
    result = jinja2.Template( """
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >  _activeParticles;
  std::vector< int >                             _numberOfActiveParticlesAdded;
""")
    return result.render(**self.d)
