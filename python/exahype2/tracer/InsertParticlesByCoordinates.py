# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer



class InsertParticlesByCoordinates(ActionSet):
  def __init__(self,particle_set,coordinates):
    """

    particle_set: ParticleSet
 
    N: number of particles

    coordinates: [ [Float,Float,Float] ]
      Coordinates for the particle. So you can write 
      
            coordinates=[ [0.0,0.0,0.0], [0.2,0.2,0.0]]
            
      for example to insert two particles. The code will create two particles.
      This variant will work with 2d and 3d compiles, where the third 
      coordinate is ignored if you compile in 2d. If you pass in 2d coordinates,
      then the code will fail (assertion) if you try to compile it in 3d.

    """

    self.d = {}
    self.d[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.d[ "N" ]                        = len(coordinates)
    self._coordinates			         = coordinates
    

  __Template_TouchCellFirstTime = jinja2.Template("""
  for(int i=0;i<{{N}};i++){
    // It is important to have this asymmetric comparisons with <= as we
    // need to ensure that particles right in the centre are either associated
    // with the vertex left or right.
    if (
      not marker.willBeRefined() 
      and 
      (marker.getOffset()(0) <= coordinates[i][0] and marker.getOffset()(0)+marker.h()(0) > coordinates[i][0])
      and
      (marker.getOffset()(1) <= coordinates[i][1] and marker.getOffset()(1)+marker.h()(1) > coordinates[i][1])
      #if Dimensions==3
      and
      (marker.getOffset()(2) <= coordinates[i][2] and marker.getOffset()(2)+marker.h()(2) > coordinates[i][2])
      #endif
    )
    {
      globaldata::{{PARTICLE}}* newParticle = new globaldata::{{PARTICLE}}();
      newParticle->setNumber(0, _spacetreeId);
      newParticle->setNumber(1, _particleNumberOnThisTree);
      #if Dimensions==2
      toolbox::particles::init(*newParticle,{coordinates[i][0],coordinates[i][1]},0.0);
      #else
      toolbox::particles::init(*newParticle,{coordinates[i][0],coordinates[i][1],coordinates[i][2]},0.0);
      #endif
      _particleNumberOnThisTree++;
      logInfo( "touchVertexFirstTime(...)", "insert " << _particleNumberOnThisTree << "th particle at " << newParticle->toString() );

      toolbox::particles::insertParticleIntoCell(
        marker,
        newParticle,
        fineGridVertices{{PARTICLES_CONTAINER}}
      );
    }
  }
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.render(**self.d)
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
#include "toolbox/particles/ParticleFactory.h"
""" )
    return result.render(**self.d)


  def Read_In_Coor(self):
    result=""
    dimensions = -1
    for i in range(self.d["N"]):
      if len(self._coordinates[i])==3:
        result+= " coordinates["+str(i)+"][0]="+str(self._coordinates[i][0])+";"
        result+= " coordinates["+str(i)+"][1]="+str(self._coordinates[i][1])+";"
        result+= " #if Dimensions==3"
        result+= " coordinates["+str(i)+"][2]="+str(self._coordinates[i][2])+";"
        result+= " #endif"
        result+= "\n"
        assert dimensions==-1 or dimensions==3, "inconsistent data: all coordinates have to have three entries: {}".format(self._coordinates[i])
        dimensions = 3
      if len(self._coordinates[i])==2:
        result+= " coordinates["+str(i)+"][0]="+str(self._coordinates[i][0])+";"
        result+= " coordinates["+str(i)+"][1]="+str(self._coordinates[i][1])+";"
        result+= " assertion(Dimensions==2);"
        result+= "\n"
        assert dimensions==-1 or dimensions==2, "inconsistent data: all coordinates have to have three entries: {}".format(self._coordinates[i])
        dimensions = 2
    return result


  def get_constructor_body(self):
    return f"""
  _particleNumberOnThisTree = 0;
  _spacetreeId              = treeNumber;
{self.Read_In_Coor()}
"""


  def get_attributes(self):
     return f"""
  int _particleNumberOnThisTree;
  int _spacetreeId;
  double coordinates["""+str(self.d["N"])+"""][3];
"""
