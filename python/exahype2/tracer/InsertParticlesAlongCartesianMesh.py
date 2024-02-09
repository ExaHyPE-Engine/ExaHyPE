# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2



class InsertParticlesAlongCartesianMesh(peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells):
  """
  
  Basically superclass, though we add these numbers.
  
  """  
  
  
  def __init__(self,
               particle_set,
               distance_between_particles,
               computational_domain_offset,
               noise=True,
               additional_includes="",
               guard="true"
               ):
    """
 
    See superclass

    """
    super( InsertParticlesAlongCartesianMesh,self ).__init__(self,
               particle_set,
               distance_between_particles,
               computational_domain_offset,
               f"""
      particle->setNumber(0,_spacetreeId);
      particle->setNumber(1,_particleNumberOnThisTree);      
      particle->setMoveState(globaldata::{particle_set.particle_model.name}::MoveState::NotMovedYet);

      _particleNumberOnThisTree++;
      
      logInfo( "touchVertexFirstTime(...)", "insert particle " << particle->getX() );
      """, # initialisation_call,
               noise,
               additional_includes + """  
#include "Constants.h"
""",
               guard
               )


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_constructor_body(self):
     return super( InsertParticlesAlongCartesianMesh ).get_constructor_body() + """
  _particleNumberOnThisTree = 0;
  _spacetreeId              = treeNumber;
"""


  def get_attributes(self):
     return super( InsertParticlesAlongCartesianMesh ).get_attributes() + """
  int _particleNumberOnThisTree;
  int _spacetreeId;
"""


