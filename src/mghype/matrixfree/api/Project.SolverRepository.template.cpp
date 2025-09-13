#include "SolverRepository.h"
#include "exahype2/LoadBalancingConfiguration.h"
#include "peano4/grid/grid.h"
#include "tarch/multicore/Core.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/orchestration/Hardcoded.h"


{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    static tarch::logging::Log _log( "{{NAMESPACE | join("::")}}" );

    int plotCounter = 0;

    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}

    {% if LOAD_BALANCER != "" %}
    {{LOAD_BALANCER}}  loadBalancer({{LOAD_BALANCER_ARGUMENTS}});
    {% else %}
    toolbox::loadbalancing::strategies::NoLoadBalancing  loadBalancer;
    {% endif %}
    {% for SOLVER in AUX_SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}

    bool terminationCriterionHolds() {
    /*
    Return true if all solvers returns true.

    We don't test the aux solvers here.
    */
      bool output = true;
      {% for SOLVER in SOLVER_INSTANCES %}
      output &= {{SOLVER[1]}}.terminationCriterionHolds();
      {% endfor %}
      return output;
    }


    void beginMeshSweep() {
      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.beginMeshSweep();
      {% endfor %}
      {% for SOLVER in AUX_SOLVER_INSTANCES %}
      {{SOLVER[1]}}.beginMeshSweep();
      {% endfor %}
    }

    void endMeshSweep() {
      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.endMeshSweep();
      {% endfor %}
      {% for SOLVER in AUX_SOLVER_INSTANCES %}
      {{SOLVER[1]}}.endMeshSweep();
      {% endfor %}
    }

    void beginCreateGrid() {
      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.beginGridConstructionSweep();
      {% endfor %}
    }

    void endCreateGrid() {
      logDebug( "endCreateGrid", "switch due to create grid" );
      loadBalancer.finishStep();
      tarch::multicore::setOrchestration( {{MULTICORE_ORCHESTRATION}} );
    }
    
    void beginInitialisation() {
      loadBalancer.enable(false);
      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.beginInitialisationSweep();
      {% endfor %}
    }

    void endInitialisation() {
      loadBalancer.finishStep();
    }

    void beginPlot() {
      {% for SOLVER in SOLVER_INSTANCES %}
      {{SOLVER[1]}}.beginPlotSweep();
      {% endfor %}
    }

    void endPlot() {
      plotCounter++;
    }

    int getPlotCounter() {
      return plotCounter;
    }
  }
{% for N in NAMESPACE %}
}
{% endfor %}

