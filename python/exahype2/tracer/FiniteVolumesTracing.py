# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2



class FiniteVolumesTracing(peano4.toolbox.particles.ParticleParticleInteraction):
  def __init__(self,particle_set,solver,velocity_indices,data_indices,scaling_of_velocities,time_stepping_kernel="toolbox::particles::explicitEulerWithoutInterpolation"):
    """

    Very simple particle seed which inserts particles into every unrefined
    cell. Logically, the particles align along a Cartesian mesh with mesh 
    size h even though I insert at least one particle per cell.

    particle_set: ParticleSet
 
    h: Float
     Initial spacing of particles. Use -1 if you want to have one particle
     per cell.

    velocity_indices: [Int]
     Indices within voxel field that denote velocity. So if the entries 4,6,8
     from a field define the velocities (we start counting with 0), then you 
     have to pass [4,6,8]. Hand in the empty set if you tracer particles shall
     not move.
     
    data_indices: [Int]
     Similar to velocity indices. Cardinality of index set has to match the 
     tracer's data cardinality, i.e. the number of unknowns the tracer shall
     track.

    """
    
    self.tracerDict = {}
    # Don't overwrite hte dictionary of hte superclass. We'll need it
    self.tracerDict[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.tracerDict[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.tracerDict[ "SOLVER_NAME" ]              = solver._name
    self.tracerDict[ "SOLVER_INSTANCE" ]          = solver.get_name_of_global_instance()
    self.tracerDict[ "PATCH_SIZE" ]               = solver._patch_size
    self.tracerDict[ "NUMBER_OF_UNKNOWNS" ]             = solver._unknowns
    self.tracerDict[ "NUMBER_OF_AUXILIARY_VARIABLES" ]  = solver._auxiliary_variables
    self.tracerDict[ "VELOCITY_INDICES" ]               = velocity_indices
    self.tracerDict[ "DATA_INDICES" ]                   = data_indices
    self.tracerDict[ "SCALE_VELOCITIES" ]               = scaling_of_velocities
    self.tracerDict[ "TIME_STEPPING_KERNEL" ]           = time_stepping_kernel
    
    
    # There should be a check whether the list lenght and the no of 
    # data points does match. But I don't know how to get this data
    # easily.
    #if len(data_indices)!=particle_set.particle_model.
    
    cell_compute_kernel = """
//Run over local set
if ( not marker.isRefined() ) {
  for (auto& p: localParticles) {
    if ( 
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::NotMovedYet
      and
      marker.isContained( p->getX() )
    ) {
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::Moved;
"""      

    if velocity_indices != []:
      cell_compute_kernel += """
      #if Dimensions==2
      tarch::la::Vector<Dimensions,double> x = {{TIME_STEPPING_KERNEL}}(
        marker,
        {{PATCH_SIZE}},
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
        { {{VELOCITY_INDICES[0]}},{{VELOCITY_INDICES[1]}} },
        _timeStepSize * {{SCALE_VELOCITIES}},
        fineGridCell{{SOLVER_NAME}}Q.value,
        p->getX()
      );
      #else
      tarch::la::Vector<Dimensions,double> x = {{TIME_STEPPING_KERNEL}}(
        marker,
        {{PATCH_SIZE}},
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
        { {{VELOCITY_INDICES[0]}},{{VELOCITY_INDICES[1]}},{{VELOCITY_INDICES[2]}} },
        _timeStepSize * {{SCALE_VELOCITIES}},
        fineGridCell{{SOLVER_NAME}}Q.value,
        p->getX()
      );
      #endif
      p->setX( x );
"""

      
    if data_indices!=[]:
      cell_compute_kernel += """
      tarch::la::Vector<Dimensions,int> voxel = toolbox::particles::mapParticleOntoVoxel(marker,{{PATCH_SIZE}},p->getX());
      int voxelIndex = peano4::utils::dLinearised(voxel,{{PATCH_SIZE}});
      double* Q      = fineGridCell{{SOLVER_NAME}}Q.value + voxelIndex * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});
      tarch::la::Vector<""" + str(len(data_indices)) + """,double> data;
"""
      data_counter = 0
      for i in data_indices:
        cell_compute_kernel += """
      data(""" + str(data_counter) + """) = Q[""" + str(i) + """];
"""
        data_counter += 1
    
      cell_compute_kernel += """
      p->setData(data);
"""

    
    cell_compute_kernel += """
    }
  }
}
"""

    touch_vertex_first_time_compute_kernel = """
  for (auto& p: localParticles) {
    p->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
  }
"""

    peano4.toolbox.particles.ParticleParticleInteraction.__init__(self,
      particle_set,
      jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
      jinja2.Template(touch_vertex_first_time_compute_kernel).render(**self.tracerDict)
    )


 
  __Template_BeginIteration = jinja2.Template("""
  _timeStepSize = repositories::{{SOLVER_INSTANCE}}.getMaxTimeStepSize();
""")


  def get_body_of_operation(self,operation_name):
    result = peano4.toolbox.particles.ParticleParticleInteraction.get_body_of_operation(self,operation_name)
    if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = self.__Template_BeginIteration.render(**self.tracerDict)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
    

  def user_should_modify_template(self):
    return False


  def get_includes(self):
    result = jinja2.Template( """
#include "tarch/multicore/Lock.h"
#include "peano4/utils/Loop.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/Tracer.h"
#include "repositories/SolverRepository.h"
""" )
    return peano4.toolbox.particles.ParticleParticleInteraction.get_includes(self) + "\n" + result.render(**self.d)


  def get_attributes(self):
    return peano4.toolbox.particles.ParticleParticleInteraction.get_attributes(self) + """
double _timeStepSize;    
"""
