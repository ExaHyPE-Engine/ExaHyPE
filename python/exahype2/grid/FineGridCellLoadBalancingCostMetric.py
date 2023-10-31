# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4.datamodel.DaStGen2
import jinja2



class FineGridCellLoadBalancingCostMetric(ActionSet):
    """!
    
    Load balancing metric which assigns each cell a value
    
    To use this metric, you have to do a couple of steps:
    
    1. Create a new label via create_cell_label(). Capture the result object.
    2. Create a new instance of this class.
    3. Add the metric to the algorithmic step of your choice. A typical set is
       ~~~~~~~~~~~~~~~~~~~~~
             project.add_action_set_to_create_grid( load_balancing_action_set )
       ~~~~~~~~~~~~~~~~~~~~~


    So overall, a typical use looks similar to:
    
    
    ~~~~~~~~~~~~~~~~~~~~~
      load_balancing_cell_label = exahype2.grid.FineGridCellLoadBalancingCostMetric.create_cell_label(project)
      load_balancing_action_set = exahype2.grid.FineGridCellLoadBalancingCostMetric(load_balancing_cell_label)
      project.add_action_set_to_create_grid( load_balancing_action_set )
      project.set_load_balancing(
        "toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree",
        "new ::exahype2::LoadBalancingConfiguration(0.98, 1, " + str(args.trees) + "), new toolbox::loadbalancing::metrics::CustomCellWeight()",
        )
    ~~~~~~~~~~~~~~~~~~~~~

    """
    def __init__(self,
                 load_balancing_cell_label,
                 initialisation_weight = "1.0",
                 ):
        super( FineGridCellLoadBalancingCostMetric, self ).__init__()
        self.load_balancing_cell_label = load_balancing_cell_label
        self.initialisation_weight     = initialisation_weight
        

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


    def user_should_modify_template(self):
        return False


    def get_body_of_operation(self,operation_name):
        result = "\n"
        if operation_name==ActionSet.OPERATION_CREATE_CELL:
          result += """
  fineGridCell""" + self.load_balancing_cell_label.name + """.setWeight( 
    """ + self.initialisation_weight + """
  );
"""
        if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
          result += """
  if (not marker.willBeRefined()) {
    toolbox::loadbalancing::metrics::CustomCellWeight::logCellWeight(
      _treeNumber,
      fineGridCell""" + self.load_balancing_cell_label.name + """.getWeight()
    );
  }
"""
        return result


    def get_attributes(self):
        return """
  int _treeNumber;
"""
    

    def get_constructor_body(self):
        return """
  _treeNumber = treeNumber;
"""        


    def get_includes(self):
        return """
#include "repositories/SolverRepository.h"
#include "toolbox/loadbalancing/metrics/CustomCellWeight.h"
"""    

        
    def create_cell_label(exahype2_project,
                          name="LoadBalancingCostMetric"):
      """!
    
       Create a cell label that's then used for the load balancing
         
      """
      result = peano4.datamodel.DaStGen2( name )
      
      result.data.add_attribute( dastgen2.attributes.Double("Weight") )
        
      result.peano4_mpi_and_storage_aspect.merge_implementation = """
      switch (context) {
        case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
          assertionMsg( false, "cells should never be subject of a boundary exchange" );
          break;
        case ::peano4::grid::TraversalObserver::SendReceiveContext::ForkDomain:
          *this = neighbour;
          break;
        default:
          assertionMsg( false, "not implemented yet" );
      }
    """
    
    
      exahype2_project._project.datamodel.add_cell(result)
    
      exahype2_project.plot_solution.use_cell(result)
      exahype2_project.init_grid.use_cell(result)
      exahype2_project.perform_time_step.use_cell(result)
      exahype2_project.create_grid.use_cell(result)
      exahype2_project.create_grid_but_postpone_refinement.use_cell(result)
      exahype2_project.create_grid_and_converge_lb.use_cell(result)
      
      return result
