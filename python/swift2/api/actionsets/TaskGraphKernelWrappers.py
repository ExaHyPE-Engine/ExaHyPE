# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4
import peano4.toolbox.particles


def construct_cell_marker_for_tasks():
    return peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.create_cell_marker(
            "Swift2TaskNumber"
        )

def construct_vertex_marker_for_tasks():
    return peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.create_vertex_marker(
            "Swift2TaskNumber"
        )


def construct_touch_vertex_first_time_call(
    current_species_set,
    touch_vertex_first_time_kernel,
    use_multilevel_dependencies,
):
    """!
    
    Take the kernel to be used in touch first time and wrap into tasks
    
    As we work with task dependencies, I decided to always spawn tasks, even if
    these degenerate to empty ones. Otherwise, it just became a nightmare. 
    Notably note that the kernel is not empty in most cases, as it is enriched
    with Swift dependency tracking anyway.
    
    
    ## Multiscale task order
    
    In terms of multiscale dependencies, we impose the following order:
    
    - touchVertexFirstTime(): Touch all associated particles for the first 
      time. At this point, touchVertexFirstTime() on corresponding coarser
      levels has to be complete.
    - touchCellFirstTime(): Handle all particles within the cell. At this point
      the corresponding first touch of every particle has to be finished.
    - Recurse
    - touchVertexLastTime(): At this point, touchVertexLastTime() on all finer
      levels has to be complete. 
      
      
    ## Realisation 
    
    - Get the task number to be used (corresponding to TouchVertexFirstTime), 
      and also get the one corresponding to the TouchVertexLastTime. The latter
      task is not spawned yet, and will not be spawned by this routine, but 
      we'll gonna need this number further down.
    - Allocate the set of in dependencies.
    - We take the parent task numbers and add them to the dependencies. For 
      this, we rely entirely on the helper function 
      ::swift2::getVertexNumbersOfParentVertices(). Consult the routine's 
      documentation for further info.
    - We make the new vertex task depend on its touch last counterpart. This 
      way, we avoid that the
      previous mesh traversal did not synchronise all tasks (task graph 
      approach) and this vertex task might still be pending.

    The only tricky or unexpected part here is likley the wrap-around 
    dependency, i.e. the fact that the task depends upon its predecessor
    from the previous mesh sweep (touchVertexLastTime).
    
    """
    d = {
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES": use_multilevel_dependencies,
        "TOUCH_VERTEX_FIRST_TIME_KERNEL": touch_vertex_first_time_kernel,
    }
    return jinja2.Template(
        """
  const ::swift2::TaskNumber taskNumberToBeUsed{
      fineGridVertex{{MARKER_NAME}}.getNumber(),
      ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
    };
              
  {% if USE_MULTILEVEL_DEPENDENCIES %}
  std::set<::swift2::TaskNumber> inDependencies = ::swift2::getVertexNumbersOfParentVertices(
    marker,
    coarseGridVertices{{MARKER_NAME}},
    ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
  );
  {% else %}
  std::set<::swift2::TaskNumber> inDependencies;  
  {% endif %}

  const ::swift2::TaskNumber taskNumberOfCorrespondingTouchLastTask{
      fineGridVertex{{MARKER_NAME}}.getNumber(),
      ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
    };

  inDependencies.insert( taskNumberOfCorrespondingTouchLastTask );

  logDebug( 
    "touchVertexFirstTime(...)", 
    "create task " << taskNumberToBeUsed.toString() << 
    " for " << marker.toString() <<
    " depends on " << ::swift2::toString(parentTaskNumbers) <<
    " with " << assignedParticles.size() << " particles on marker " << marker.toString() 
  );

  tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor (
    tarch::multicore::Task::DontFuse,
    tarch::multicore::Task::DefaultPriority,
    [=,this]()->bool {
      {% if TOUCH_VERTEX_FIRST_TIME_KERNEL!=None %}
      {{TOUCH_VERTEX_FIRST_TIME_KERNEL}}
      {% endif %}
      return false;
    }
  );
  
  tarch::multicore::spawnTask( newTask, ::swift2::flatten(inDependencies), ::swift2::flatten(taskNumberToBeUsed) );
"""
    ).render(**d)


def construct_touch_cell_first_time_call(
    current_species_set,
    touch_cell_first_time_kernel,
    use_multilevel_dependencies,
):
    """!
    
    Wrap cell kernel into task
    
    Consult the documentation of construct_touch_vertex_first_time_call()
    for a discussion of the task orders.
    
    In principle, the handling cells is straightforward: We take the @f$ 2^d @f$ adjacent 
    vertices and exatract their task numbers, and then we also add an 
    additional task number for the parent cell if we have multi-level
    dependencies.
    
    
    """
    d = {
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES":   use_multilevel_dependencies,
        "TOUCH_CELL_FIRST_TIME_KERNEL":  touch_cell_first_time_kernel,
    }
    return jinja2.Template(
        """
      const ::swift2::TaskNumber taskNumberToBeUsed{
        fineGridCell{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchCellFirstTime
      };
      
      int adjacentResourcesIndices[TwoPowerD];
      std::set<::swift2::TaskNumber> adjacentVertexAndParentTasks;
      for (int i=0; i<TwoPowerD; i++) {
        adjacentResourcesIndices[i] = fineGridVertices{{MARKER_NAME}}(i).getNumber();
        adjacentVertexAndParentTasks.insert( ::swift2::TaskNumber{
          fineGridVertices{{MARKER_NAME}}(i).getNumber(),
          ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
        } );
      }
      
      {% if USE_MULTILEVEL_DEPENDENCIES %}
      const ::swift2::TaskNumber  parentTaskNumber = ::swift2::TaskNumber(
        coarseGridCell{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchCellFirstTime
      );
      adjacentVertexAndParentTasks.insert( parentTaskNumber );
      {% endif %}

      logDebug( 
        "touchCellFirstTime(...)", 
        "create task " << taskNumberToBeUsed.toString() << 
        " for " << marker.toString() <<
        " depends on " << ::swift2::toString(adjacentVertexAndParentTasks) <<
        " with " << localParticles.size() << " local particles on marker " << marker.toString() <<
        " on tree " << _spacetreeId
      );
      
      tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor (
        tarch::multicore::Task::DontFuse,
        tarch::multicore::Task::DefaultPriority,
        [=,this]()->bool {
          ::swift2::TaskEnumerator::lockResources(adjacentResourcesIndices);
        
          {% if TOUCH_CELL_FIRST_TIME_KERNEL!=None %}
          {{TOUCH_CELL_FIRST_TIME_KERNEL}}
          {% endif %}
          
          ::swift2::markAllParticlesAsUpdatedWithinCell( localParticles, marker );
          
          ::swift2::TaskEnumerator::unlockResources(adjacentResourcesIndices);
          
          return false;
        }
      );  
      tarch::multicore::spawnTask( newTask, ::swift2::flatten(adjacentVertexAndParentTasks), ::swift2::flatten(taskNumberToBeUsed) );
    """
    ).render(**d)


def construct_touch_vertex_last_time_call(
    current_species_set,
    touch_vertex_last_time_kernel,
    use_multilevel_dependencies,
    alter_particle_position_or_global_state,
):
    """!
    
    Wrap vertex kernel into task
    
    Consult the documentation of construct_touch_vertex_first_time_call()
    for a discussion of the task orders.
    Multiple things have to be taken into account here:
    
    - We add dependencies to all @f$ 2^d @f$ adjacent cells. Their tasks
      have to be completed before we start our work. This is notably 
      important if we are on the finest mesh level, i.e. the marker is
      not refined. But it never hurts to add these dependendices.
    - We then add dependencies for finer levels (see discussion below).

    Once these two types of dependencies are in, we can spawn the task.
    
    The tricky part in this routine is the identification of fine grid
    dependencies. We don't have access to the fine grid data at this 
    point anymore. So what we have to do instead is memorising the 
    dependencies that will be established from fine to coarse on the 
    finer level. We use
    
      std::set<::swift2::TaskNumber> parentTasks = getVertexNumbersOfParentVertices(
    
    to find out the tasks of the up to @f$ 2^d @f$ parent tasks. Those 
    are not yet spawned, as their touchVertexLastTime() will come later 
    when we ascend in the tree. Therefore, we memorise that we have to 
    add these later as in-dependencies in the underlying action set's
    _pendingDependencies attribute.
    
    
    """
    d = {
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES":             use_multilevel_dependencies,
        "ALTER_PARTICLE_POSITION_OR_GLOBAL_STATE": alter_particle_position_or_global_state,
        "TOUCH_VERTEX_LAST_TIME_KERNEL":           touch_vertex_last_time_kernel,
        "SPECIES_SET_NAME":                        current_species_set.name
    }
    return jinja2.Template(
        """
      std::set<::swift2::TaskNumber> inDependencies;
      
      const ::swift2::TaskNumber taskNumberToBeUsed{
        fineGridVertex{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
      };

      // handle finer levels      
      {% if USE_MULTILEVEL_DEPENDENCIES %}
      inDependencies = ::swift2::getDependenciesForTask( 
        taskNumberToBeUsed, 
        _pendingDependencies 
      );
      {% endif %}

      // add tasks from neighbour cells
      for (int i=0; i<TwoPowerD; i++) {
        inDependencies.insert( ::swift2::TaskNumber{
          fineGridVertex{{MARKER_NAME}}.getAdjacentCellNumber(i),
          ::swift2::TaskNumber::TaskAssociation::TouchCellFirstTime
        } );
      }
      
      logDebug( 
        "touchVertexLastTime(...)", 
        "create task " << taskNumberToBeUsed.toString() << 
        " for " << marker.toString() <<
        " depends on " << ::swift2::toString(inDependencies) <<
        " with " << assignedParticles.size() << " particles on marker " << marker.toString() 
      );

      tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor (
        tarch::multicore::Task::DontFuse,
        tarch::multicore::Task::DefaultPriority,
        [=,this]()->bool {
          {% if TOUCH_VERTEX_LAST_TIME_KERNEL!=None %}
          {{TOUCH_VERTEX_LAST_TIME_KERNEL}}
          {% endif %}
          return false;
        }
      );

      tarch::multicore::spawnTask( newTask, ::swift2::flatten(inDependencies), ::swift2::flatten(taskNumberToBeUsed) );

      {% if USE_MULTILEVEL_DEPENDENCIES %}
      std::set<::swift2::TaskNumber> parentTasks = getVertexNumbersOfParentVertices(
        marker,
        coarseGridVertices{{SPECIES_SET_NAME}}EnclaveMeshEntityNumber,
        ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
      );
      for (auto p: parentTasks) {
        _pendingDependencies.insert( std::pair<::swift2::TaskNumber, ::swift2::TaskNumber>(
            taskNumberToBeUsed, p
        ));
      }
      {% endif %}
      
      {% if ALTER_PARTICLE_POSITION_OR_GLOBAL_STATE %}
      constexpr bool alterParticlePositionOrGlobalState = true;
      {% else %}
      constexpr bool alterParticlePositionOrGlobalState = false;
      {% endif %}

      if ( marker.isAdjacentToParallelDomainBoundary() or alterParticlePositionOrGlobalState ) {
        tarch::multicore::waitForTask( taskNumberToBeUsed.flatten() );
      }
    """
    ).render(**d)
