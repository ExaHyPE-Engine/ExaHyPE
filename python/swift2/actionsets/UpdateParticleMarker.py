# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class UpdateParticleMarker(ActionSet):
  def __init__(self, 
               particle_set):
    """
    
    Update particles marker

    Particles do not uniquely belong to one cell or the other. If they sit
    exactly on the face in-between two cells, they belong to both. If they 
    sit on a vertex location, they can belong to up to 2^d cells. We cannot
    avoid such an ambivalency, as we work with floating point numbers, and
    as particles are stored within the vertices. If they were stored within 
    cells, the association would be unique by definition.
    
    Therefore, we introduce a boolean per particle which indicates if a 
    particle has been updated by a cell already. That is, we do not make 
    the association unique artificially, but we realise a "whoever grabs it
    first"-policy.

    The realisation of this marker is straightforward: The 
    swift2.particle.Particle base class defines the marker, which is a simple
    bool. When we touch a vertex for the first time, we implicitly also touch
    all particles associated with this vertex for the first time. So we set 
    the marker to false.
    
    When we leave a cell, we set all the flags to true. We can be sure that 
    this routine is called before we store the vertices and we can be sure that
    it is called before we enter any adjacent cell. This statement implies that
    you shall not change a vertex position prior to touchVertexLastTime().

    """
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                    = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]         = particle_set.name
    

  __Template_TouchVertexFirstTime = jinja2.Template("""
  auto& particles  = fineGridVertex{{PARTICLES_CONTAINER}};

  for (auto* particle: particles) {
    particle->setCellHasUpdatedParticle(false);
  }
""")


  __Template_TouchCellLastTime = jinja2.Template("""
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* particle: fineGridVertices{{PARTICLES_CONTAINER}}(i) ) {
      if ( marker.isContained( particle->getX() ) ) {
        particle->setCellHasUpdatedParticle(true);
      }
    }
  }
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = self.__Template_TouchVertexFirstTime.render(**self.d)
    return result


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n"


  #def get_body_of_prepareTraversal(self):  
  #  return self.d[ "PREPARE_TRAVERSAL_KERNEL" ]


  #def get_body_of_unprepareTraversal(self):
  #  return self.d[ "UNPREPARE_TRAVERSAL_KERNEL" ]


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_includes(self):
    return jinja2.Template( """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
""" ).render( **self.d )

