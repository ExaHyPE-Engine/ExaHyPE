# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2
import peano4


class UpdateParticleMarker(ActionSet):
  def __init__(self, 
               particle_set,
               automatically_set_marker_in_leave_cell,
               make_reset_separate_task,
               ):
    """!
    
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
    
    For a task-based version, we cannot set the flags to true in 
    touchCellLastTime(). At this point, the cell's task might not have run yet, 
    so if we reset the particle state, we run risk that we break the code
    semantics of the task functor, as they typically check for the update
    flag. So if we use tasks, it has to be the task itself which sets the 
    "have updated" flag to true. This is realised within the tasks created by
    TaskGraphKernelWrapper.
    
    We note that the marker's name is unfortunate: It is not really that we 
    have updated a particle necessary. But we keep track that we have at least
    studied one.

    """
    self._particle_set = particle_set
    self.d = {}
    self.d[ "PARTICLE" ]                    = particle_set.particle_model.name
    self.d[ "PARTICLES_CONTAINER" ]         = particle_set.name
    self.d[ "MARKER_NAME" ]                 = peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
       particle_set.name
    )
    self._automatically_set_marker_in_leave_cell = automatically_set_marker_in_leave_cell
    self._make_reset_separate_task               = make_reset_separate_task


    

  __Template_TouchVertexFirstTime = jinja2.Template("""
  auto& particles  = fineGridVertex{{PARTICLES_CONTAINER}};

  for (auto* particle: particles) {
    particle->setCellHasUpdatedParticle(false);
  }
""")


  __Template_TouchVertexFirstTimeAsTask = jinja2.Template("""
  auto& particles  = fineGridVertex{{PARTICLES_CONTAINER}};

  const ::swift2::TaskNumber taskNumberToBeUsed{
      fineGridVertex{{MARKER_NAME}}.getNumber(),
      ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
    };

  tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor (
    tarch::multicore::Task::DontFuse,
    tarch::multicore::Task::DefaultPriority,
    [=,this]()->bool {
      for (auto* particle: particles) {
        particle->setCellHasUpdatedParticle(false);
      }
      return false;
    }
  );
  
  tarch::multicore::spawnTask( newTask, std::set<int>{::swift2::flatten(taskNumberToBeUsed)}, ::swift2::flatten(taskNumberToBeUsed) );
""")


  __Template_TouchCellLastTime = jinja2.Template("""
  for (int i=0; i<TwoPowerD; i++) {
    ::swift2::markAllParticlesAsUpdatedWithinCell( fineGridVertices{{PARTICLES_CONTAINER}}(i), marker );
  }
""")


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if self._automatically_set_marker_in_leave_cell and operation_name==ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellLastTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME and not self._make_reset_separate_task:
      result = self.__Template_TouchVertexFirstTime.render(**self.d)
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME and self._make_reset_separate_task:
      result = self.__Template_TouchVertexFirstTimeAsTask.render(**self.d)
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
#include "swift2/swift2.h"
#include "swift2/TaskNumber.h"
""" ).render( **self.d )

