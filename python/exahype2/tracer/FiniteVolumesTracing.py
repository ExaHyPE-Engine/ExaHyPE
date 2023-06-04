# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2



class FiniteVolumesTracing(peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists):
  """!
  
  Particle tracing over the Finite Volumes solver. 

  The action set works for cell-centered Finite Differences, too.
  
  
  ## Update plug-in points
  
  We only update cells which are marked with getHasUpdated(). For enclave solvers,
  we assume that this flag is only set after a cell has updated.
  
  The class supports different ways how the data is projected onto tracer 
  attributes (both which data entries and how they are interpolated), as 
  well as various time integratiors. The only important detail here is 
  that the integration happens patch-wisely, i.e. you cannot access any 
  neighbouring patches or intermediate data from the time stepping calculations.
  
  """
  
  
  def __init__(self,
               particle_set, 
               solver, 
               time_stepping_kernel, 
               project_on_tracer_properties_kernel):
    """!

    Set up the tracing
    
    Setting up the tracing first and foremost means that we have to couple
    the tracer particle set with the solver, and then we have to specify which 
    projection kernels and which time integrator we want to use. Basically,
    we have to provide the C++ kernels which shall be used to move the tracer
    and to write new data into the tracer. 
    
    For the time stepping, Peano's particle toolbox provides some default 
    integrators. If you use stationary tracers, toolbox::particles::StaticPosition
    (handed in as string) or None do the job. See comment below.
    
    For the projection of solution data onto the tracer, the file
    exahype2/fv/Tracer.h provides a few pre-manufactured routines. Again, the 
    projection kernel here requires a plain function call, and you have to 
    ensure that this function call matches the data you actually wanna dump.

    
    ## Arguments

    particle_set: ParticleSet
 
    time_stepping_kernel: String
     Function call which does projection onto the particles and the actual 
     position update. Hand in "toolbox::particles::StaticPosition" or None
     if you don't want the particles' to move.
     
    project_on_tracer_properties_kernel: String
     Name of the routine that projects the Finite Volumes representation 
     onto the tracers' properties. Hand in None if you don't want any field
     properties to be projected on the tracer. This makes sense if the 
     velocity/trajectory of a tracer already encodes all information.
     Some popular arguments for this routine are enlisted above.
     
     
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
        {{PATCH_SIZE}},
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

    peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists.__init__(self,
      particle_set,
      jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
      jinja2.Template(touch_vertex_first_time_compute_kernel).render(**self.tracerDict)
    )


  #def get_body_of_operation(self,operation_name):
  #  result = peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists.get_body_of_operation(self,operation_name)
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
#include "exahype2/fv/Tracer.h"
""" )
    return peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists.get_includes(self) + "\n" + result.render(**self.tracerDict)


  def get_attributes(self):
    return peano4.toolbox.particles.ParticleParticleInteraction_StackOfLists.get_attributes(self) + """
double _timeStepSize;    
"""
