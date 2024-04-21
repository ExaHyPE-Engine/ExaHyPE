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
    if (
        touch_vertex_first_time_kernel == None
        or touch_vertex_first_time_kernel.strip() == ""
    ):
        return ""
    else:
        d = {
            "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
                current_species_set.name
            ),
            "USE_MULTILEVEL_DEPENDENCIES": use_multilevel_dependencies,
        }
        return jinja2.Template(
            """
      const ::swift2::TaskNumber taskNumberToBeUsed{
          fineGridVertex{{MARKER_NAME}}.getNumber(),
          ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
        };
      
      {% if USE_MULTILEVEL_DEPENDENCIES %}
      std::set<::swift2::TaskNumber> parentTaskNumbers = ::swift2::getVertexNumbersOfParentVertices(
        marker,
        coarseGridVertices{{MARKER_NAME}},
        ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
      );
      {% else %}
      std::set<::swift2::TaskNumber> parentTaskNumbers;
      {% endif %}

      logDebug( 
        "touchVertexFirstTime(...)", 
        "create task " << taskNumberToBeUsed.toString() << 
        " for " << marker.toString() <<
        " depends on " << ::swift2::toString(parentTaskNumbers) <<
        " with " << assignedParticles.size() << " particles on marker " << marker.toString() 
      );
      
      auto* newTask = new ::swift2::VertexTask< decltype(assignedParticles) >(
          marker,
          assignedParticles,
          [=,this](const peano4::datamanagement::VertexMarker&  marker, decltype(assignedParticles)& assignedParticles)->void {
            logInfo( "VertexFirstTask", "have been in vertex of " << marker.toString() );
            """
            + touch_vertex_first_time_kernel
            + """
          },
          false
        );
      
      tarch::multicore::spawnTask( newTask, ::swift2::flatten(parentTaskNumbers), std::set<tarch::multicore::TaskNumber>(), ::swift2::flatten(taskNumberToBeUsed) );
    """
        ).render(**d)


def construct_touch_cell_first_time_call(
    current_species_set,
    touch_cell_first_time_kernel,
    use_multilevel_dependencies,
):
    if touch_cell_first_time_kernel == None:
        touch_cell_first_time_kernel = ""

    d = {
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES": use_multilevel_dependencies,
    }
    return jinja2.Template(
        """
      const ::swift2::TaskNumber taskNumberToBeUsed{
        fineGridCell{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchCellFirstTime
      };
                  
      std::set<::swift2::TaskNumber> adjacentVertexAndParentTasks;
      std::set<::swift2::TaskNumber> adjacentCellTasks;
      for (int i=0; i<TwoPowerD; i++) {
        adjacentVertexAndParentTasks.insert( ::swift2::TaskNumber{
          fineGridVertices{{MARKER_NAME}}(i).getNumber(),
          ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
        } );
        for (int j=0; j<TwoPowerD; j++) {
          int neighbourNumber = fineGridVertices{{MARKER_NAME}}(i).getAdjacentCellNumber(j);
          if (neighbourNumber!=fineGridCell{{MARKER_NAME}}.getNumber()) {
            adjacentCellTasks.insert( ::swift2::TaskNumber{
              fineGridVertices{{MARKER_NAME}}(i).getAdjacentCellNumber(j),
              ::swift2::TaskNumber::TaskAssociation::TouchCellFirstTime 
            } );
          }
        }
      }
      
      {% if USE_MULTILEVEL_DEPENDENCIES %}
      const ::swift2::TaskNumber  parentTaskNumber = ::swift2::TaskNumber(
        coarseGridCell{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
      );
      adjacentVertexAndParentTasks.insert( parentTaskNumber );
      {% endif %}

      logDebug( 
        "touchCellFirstTime(...)", 
        "create task " << taskNumberToBeUsed.toString() << 
        " for " << marker.toString() <<
        " depends on " << ::swift2::toString(adjacentVertexAndParentTasks) <<
        " with " << localParticles.size() << " local particles on marker " << marker.toString() 
      );
      
      auto* newTask = new ::swift2::CellTask< decltype(localParticles) >(
        marker,
        localParticles,
        activeParticles,
          [=,this](const peano4::datamanagement::CellMarker&  marker, decltype(localParticles)& localParticles, decltype(activeParticles)& activeParticles)->void {
            logInfo( "CellTask", "have been in cell of " << marker.toString() );
            #pragma omp critical (cell_task)
            {
            """
        + touch_cell_first_time_kernel
        + """}
        },
        false
      );

      tarch::multicore::spawnTask( 
        newTask, 
        ::swift2::flatten( adjacentVertexAndParentTasks ), 
        ::swift2::flatten( adjacentCellTasks ),
        ::swift2::flatten( taskNumberToBeUsed )
      );
    """
    ).render(**d)


def construct_touch_vertex_last_time_call(
    current_species_set,
    touch_vertex_last_time_kernel,
    use_multilevel_dependencies,
    alter_particle_position_or_global_state,
):
    d = {
        "MARKER_NAME": peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
            current_species_set.name
        ),
        "USE_MULTILEVEL_DEPENDENCIES": use_multilevel_dependencies,
        "ALTER_PARTICLE_POSITION_OR_GLOBAL_STATE": alter_particle_position_or_global_state,
    }
    return jinja2.Template(
        """
      const ::swift2::TaskNumber taskNumberToBeUsed{
        fineGridVertex{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
      };
                
      std::set<::swift2::TaskNumber> adjacentCellTasks;
      for (int i=0; i<TwoPowerD; i++) {
        adjacentCellTasks.insert( ::swift2::TaskNumber{
          fineGridVertex{{MARKER_NAME}}.getAdjacentCellNumber(i),
          ::swift2::TaskNumber::TaskAssociation::TouchCellFirstTime
        } );
      }
      
      {% if USE_MULTILEVEL_DEPENDENCIES %}
      std::set<::swift2::TaskNumber>  fineGridDependencies = ::swift2::getDependenciesForTask( 
        taskNumberToBeUsed, 
        _pendingDependencies 
      );
      adjacentCellTasks.insert( fineGridDependencies.begin(), fineGridDependencies.end() );
      {% endif %}

      
      logDebug( 
        "touchVertexLastTime(...)", 
        "create task " << taskNumberToBeUsed.toString() << 
        " for " << marker.toString() <<
        " depends on " << ::swift2::toString(adjacentCellTasks) <<
        " with " << assignedParticles.size() << " particles on marker " << marker.toString() 
      );
      
      auto* newTask = new ::swift2::VertexTask< decltype(assignedParticles) >(
          marker,
          assignedParticles,
          [=,this](const peano4::datamanagement::VertexMarker&  marker, decltype(assignedParticles)& assignedParticles)->void {
            logInfo( "VertexLastTask", "have been in vertex of " << marker.toString() );
            """
        + touch_vertex_last_time_kernel
        + """
          },
          false
        );
      
      tarch::multicore::spawnTask( newTask, ::swift2::flatten(adjacentCellTasks), std::set<tarch::multicore::TaskNumber>(), ::swift2::flatten(taskNumberToBeUsed) );

      {% if USE_MULTILEVEL_DEPENDENCIES %}
      for (int i=0; i<TwoPowerD; i++) {
        const ::swift2::TaskNumber parentVertex{
          coarseGridVertices{{MARKER_NAME}}(i).getNumber(),
          ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
        };
        if (parentVertex!=::swift2::TaskNumber::NoDependencies) {
          logDebug( "touchVertexFirstTime(...)", "memorise task " << taskNumberToBeUsed.toString() << " -> " << parentVertex.toString() );
          _pendingDependencies.insert( std::pair<::swift2::TaskNumber,::swift2::TaskNumber>(
            taskNumberToBeUsed, parentVertex
          ));
        }
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
