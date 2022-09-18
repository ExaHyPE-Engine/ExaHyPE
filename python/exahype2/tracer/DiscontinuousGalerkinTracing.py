# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2



class DiscontinuousGalerkinTracing(peano4.toolbox.particles.ParticleParticleInteraction):
  """
  
  Particle tracing over the DG solver. 

  Please consult the class FiniteVolumesTracing for details on the semantics
  of this type. It is an exact analogon for DG.
  
  ## Update plug-in points
  
  We only update cells which are marked with getHasUpdated(). For enclave solvers,
  we assume that this flag is only set after a cell has updated. For Runge-Kutta
  type solvers, I assume that this flag is set by the computation of the final 
  linear combination of estimates.
  
  
  
  """
  
  
  def __init__(self,
               particle_set, 
               solver, 
               time_stepping_kernel, 
               project_on_tracer_properties_kernel):
    """

    @see FiniteVolumesTracing.__init()__.
     
    """
    
    self.tracerDict = {}
    # Don't overwrite hte dictionary of hte superclass. We'll need it
    self.tracerDict[ "PARTICLE" ]                 = particle_set.particle_model.name
    self.tracerDict[ "PARTICLES_CONTAINER" ]      = particle_set.name
    self.tracerDict[ "SOLVER_NAME" ]              = solver._name
    self.tracerDict[ "SOLVER_INSTANCE" ]          = solver.get_name_of_global_instance()
    self.tracerDict[ "ORDER" ]                    = solver._basis.order
    self.tracerDict[ "NUMBER_OF_UNKNOWNS" ]             = solver._unknowns
    self.tracerDict[ "NUMBER_OF_AUXILIARY_VARIABLES" ]  = solver._auxiliary_variables
    self.tracerDict[ "TIME_STEPPING_KERNEL" ]           = time_stepping_kernel
    self.tracerDict[ "PROJECTION_KERNEL" ]              = project_on_tracer_properties_kernel   
        
    cell_compute_kernel = """
if ( 
  not marker.willBeRefined() 
  and 
  fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated() 
  and
  repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
) {
  const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
  for (auto* p: localParticles) {
    if ( 
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::NotMovedYet
    ) {
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::Moved;

      {% if PROJECTION_KERNEL!=None %}
      {{PROJECTION_KERNEL}}(
        marker,
        {{ORDER}},
        repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        fineGridCell{{SOLVER_NAME}}Q.value,
        *p
      );
      {% endif %}

      {% if TIME_STEPPING_KERNEL!=None %}
      {{TIME_STEPPING_KERNEL}}(
        *p,
        timeStepSize
      );
      {% endif %}
    }
  }
}
"""

    touch_vertex_first_time_compute_kernel = """
  for (auto* p: localParticles) {
    p->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
  }
"""

    peano4.toolbox.particles.ParticleParticleInteraction.__init__(self,
      particle_set,
      jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
      jinja2.Template(touch_vertex_first_time_compute_kernel).render(**self.tracerDict)
    )


  #def get_body_of_operation(self,operation_name):
  #  result = peano4.toolbox.particles.ParticleParticleInteraction.get_body_of_operation(self,operation_name)
  #  if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
  #    result = self.__Template_BeginIteration.render(**self.tracerDict)
  #  return result


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
#include "exahype2/dg/Tracer.h"
""" )
    return peano4.toolbox.particles.ParticleParticleInteraction.get_includes(self) + "\n" + result.render(**self.tracerDict)


  def get_attributes(self):
    return peano4.toolbox.particles.ParticleParticleInteraction.get_attributes(self) + """
double _timeStepSize;    
"""
