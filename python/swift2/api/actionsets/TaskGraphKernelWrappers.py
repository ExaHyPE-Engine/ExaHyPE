# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4
import peano4.toolbox.particles


def construct_touch_vertex_first_time_call(
    current_species_set,
    touch_vertex_first_time_kernel,
    use_multilevel_dependencies,
):
    """!
    
    Take the kernel to be used in touch first time and wrap into tasks
    
    As we work with task dependencies, I decided to always spawn tasks, even if
    these degenerate to empty ones as no particles are associated to a vertex. 
    Otherwise, it just became a nightmare. 
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
    
    - Get the task number to be used (corresponding to TouchVertexFirstTime). 
    - Allocate the set of in dependencies from coarser level if a multiscale
      algorithm is used.
    - Issue task.  
    
    If we build up one task graph per algorithmic step, there's no particularly
    interesting details left: The underlying observer with its action sets will
    yield a touch first-vertex first-vertex first-cell-vertex last-vertex last
    sequence. The last "vertex last" event will be followed by a global 
    waitForAllTasks() call, and therefore we are ready to for the subsequent 
    mesh sweep. All the "vertex first" calls will use the same task number.
    The second "vertex first" task therefore will have an in-out dependency
    on this number.
    
    If we roll around, i.e. build up a task graph over multiple mesh sweeps, no
    such strategy will work, as we have to insert a "vertex last"-"vertex first"
    dependency in-between any two sweeps. This fact is ignored here and left 
    to the user. Indeed, consult MultisweepTaskGraph for details on how they 
    handle these wrap-around dependencies.
    
    """
    d = {
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES": use_multilevel_dependencies,
        "TOUCH_VERTEX_FIRST_TIME_KERNEL": touch_vertex_first_time_kernel,
        "SPECIES_SET_TASK_MARKER_NAME":   peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(current_species_set.name)
    }
    return jinja2.Template(
        """
  assertion1(
      fineGridVertex{{MARKER_NAME}}.getNumber()>=0,
      fineGridVertex{{MARKER_NAME}}.toString()
  );
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
  
  logDebug( 
    "touchVertexFirstTime(...)", 
    "create task " << taskNumberToBeUsed.toString() << 
    " (" << ::swift2::flatten(taskNumberToBeUsed) << ") " <<
    " for " << marker.toString() <<
    " depends on " << ::swift2::toString(inDependencies) <<
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
    #if PeanoDebug>0
    , "touch-vertex-first-time:" + marker.toString()
    #endif
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
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES":   use_multilevel_dependencies,
        "TOUCH_CELL_FIRST_TIME_KERNEL":  touch_cell_first_time_kernel,
        "SPECIES_SET_TASK_MARKER_NAME":  peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(current_species_set.name)
    }
    return jinja2.Template(
        """
      assertion1(
        fineGridCell{{MARKER_NAME}}.getNumber()>=0,
        fineGridCell{{MARKER_NAME}}.toString()
      );  
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
        #if PeanoDebug>0
        , "cell:" + marker.toString()
        #endif
      );  
      tarch::multicore::spawnTask( newTask, ::swift2::flatten(adjacentVertexAndParentTasks), ::swift2::flatten(taskNumberToBeUsed) );
    """
    ).render(**d)



def construct_touch_vertex_last_time_call(
    current_species_set,
    touch_vertex_last_time_kernel,
    use_multilevel_dependencies,
    alter_particle_position_or_interaction_radius,
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
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES":             use_multilevel_dependencies,
        "ALTER_PARTICLE_POSITION_OR_INTERACTION_RADIUS": alter_particle_position_or_interaction_radius,
        "TOUCH_VERTEX_LAST_TIME_KERNEL":           touch_vertex_last_time_kernel,
        "SPECIES_SET_NAME":                        current_species_set.name,
        "SPECIES_SET_TASK_MARKER_NAME":            peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(current_species_set.name)
    }
    return jinja2.Template(
        """
      std::set<::swift2::TaskNumber> inDependencies;
      
      assertion1(
        fineGridVertex{{MARKER_NAME}}.getNumber()>=0,
        fineGridVertex{{MARKER_NAME}}.toString()
      );
      
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

      // - for multiscale algorithms, there's the finer indices, which might add to the counter and hence weaken the assertion
      // - there is at least one proper adjacent cell, i.e. a local one with a valid counter
      // - the other ones might all be undef, so they enter the set once; or they might be proper indices and increase the count
      assertion2( inDependencies.size()>=2, marker.toString(), ::swift2::toString(inDependencies) );
      
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
        #if PeanoDebug>0
        , "touch-vertex-last-time:" + marker.toString()
        #endif
      );

      tarch::multicore::spawnTask( newTask, ::swift2::flatten(inDependencies), ::swift2::flatten(taskNumberToBeUsed) );

      {% if USE_MULTILEVEL_DEPENDENCIES %}
      std::set<::swift2::TaskNumber> parentTasks = getVertexNumbersOfParentVertices(
        marker,
        coarseGridVertices{{SPECIES_SET_TASK_MARKER_NAME}},
        ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
      );
      for (auto p: parentTasks) {
        _pendingDependencies.insert( std::pair<::swift2::TaskNumber, ::swift2::TaskNumber>(
            taskNumberToBeUsed, p
        ));
      }
      {% endif %}
      
      {% if ALTER_PARTICLE_POSITION_OR_INTERACTION_RADIUS %}
      constexpr bool alterParticlePositionOrInteractionRadius = true;
      {% else %}
      constexpr bool alterParticlePositionOrInteractionRadius = false;
      {% endif %}

      if ( marker.isAdjacentToParallelDomainBoundary() or alterParticlePositionOrInteractionRadius ) {
        tarch::multicore::waitForTask( taskNumberToBeUsed.flatten() );
        logDebug(
          "touchVertexLastTime(...)",
          "task " << taskNumberToBeUsed.toString() <<
          " has terminated"
        );
      }
    """
    ).render(**d)
