# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet


import peano4
import jinja2
import dastgen2


"""

  When we solve the Poisson equation for a marker, we basically
  solve  
  
  @f$
 - \Delta \alpha _{\text{DG}} = C _{\text{DG}} 
 \qquad
 \text{s.t.} \ 
 0 \leq \alpha _{\text{DG}} \leq 1, \ 
 \alpha _{\text{DG}} | _{\partial \Omega} = 0
  @f$
  
  
  ## Usage
  
  - Invent a name for the label. I often use simply 
    
          marker = "AlphaDG"
          
    but there might be better names.
  - Augment your cell label:
  
          exahype2.exahype2.solvers.rkdg.actionsets.PoissonEquationForMarker.augment_cell_label( dg_solver._cell_label, marker_name )
          
    From hereon, every cell holds all required data.
  - Augment your face label:
  
          exahype2.exahype2.solvers.rkdg.actionsets.PoissonEquationForMarker.augment_face_label( dg_solver._face_label, marker_name )
          
    We will implement a cell-wise finite differences scheme, but we cannot 
    access neighbouring cells within Peano. Therefore, we have to map the 
    quantities to the face.

  
"""
def augment_cell_label( cell_label, marker_name ):
    """

    Give the cell label a new attribute called marker_name.    
    
    """
    cell_label.data.add_attribute(dastgen2.attributes.Double(marker_name))
    pass


  def augment_face_label_update_action_set( action_set_update_face_label,
                                            marker_name ):
    """

    We take the action set that updates the face label (and does 
    other fancy stuff) and we augment it in various ways:
    
    - When we touch an average for the first time, we have to roll over the
      new value into the old field.
    - After that, we clear the new field.
    - When we create a face label, we set both average fields to zero.
    
    """
    action_set_update_face_label._Template_CreateFace += """
  fineGridFace{{FACE_LABEL_NAME}}.setOldAverage""" + marker_name + """( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAverage""" + marker_name + """( 0.0 );
"""
    action_set_update_face_label._Template_CreateHangingFace += """
  double markerValue = coarseGridFaces{{FACE_LABEL_NAME}}( marker.getSelectedFaceNumber() ).getOldAverage""" + marker_name + """();

  fineGridFace{{FACE_LABEL_NAME}}.setOldAverage""" + marker_name + """( markerValue );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAverage""" + marker_name + """( markerValue );
"""
    action_set_update_face_label._Template_TouchFaceFirstTime += """
  fineGridFace{{FACE_LABEL_NAME}}.setOldAverage""" + marker_name + """( fineGridFace{{FACE_LABEL_NAME}}.getNewAverage""" + marker_name + """() );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAverage""" + marker_name + """( 0.0 );
"""
    action_set_update_face_label._Template_DestroyHangingFace += """
  coarseGridFaces{{FACE_LABEL_NAME}}( marker.getSelectedFaceNumber() ).setNewAverage""" + marker_name + """(
    coarseGridFaces{{FACE_LABEL_NAME}}( marker.getSelectedFaceNumber() ).getNewAverage""" + marker_name + """()
    +
    fineGridFace{{FACE_LABEL_NAME}}.getNewAverage""" + marker_name + """()
  );
"""
    pass


  def augment_face_label( face_label, 
                          marker_name ):
    """

    Augment the face label so we can solve the Poisson equation
    
    First, we add a new and old average value to the face. These fields will 
    hold the averages of the label within the cells.
    
    Second, we add some merge routines that accumulate the new averages. For
    both the first and the second step, we need access to the face label and
    we have to know what we should call the marker.
    
    """
    face_label.data.add_attribute(dastgen2.attributes.Double("OldAverage" + marker_name))
    face_label.data.add_attribute(dastgen2.attributes.Double("NewAverage" + marker_name))

    face_label.peano4_mpi_and_storage_aspect.merge_implementation += """
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        _NewAverage""" + marker_name + """ += neighbour._NewAverage""" + marker_name + """;
      }
      break;
    default:
      assertionMsg( false, "not implemented yet" );
  }
"""
    pass

  
  def __init__(self,solver):
    super(PoissonEquationForMarker,self).__init__(solver)
    self.guard                            = "true"
    self._compute_kernel                  = ""


  def add_postprocessing_kernel(self, operation_per_cell):
    """
    
    Add a code snippet which is applied to each and every point. You have the following
    variables which are well-defined:
    
    - value: Is a pointer to the current finite volume's data
    - x: A tarch::la::Vector over doubles
    - marker.h(): A tarch::la::Vector over doubles
    
    
    operation_per_point: String
      C/C++ code snippet
    
    """
    self._compute_kernel += """
{    
  {{COMPUTE_TIME_STEP_SIZE}}
  const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    
  if (
    not marker.hasBeenRefined() 
    and 
    {{PREDICATE}}
  ) { 
    logTraceIn( "touchCellFirstTime(...)" );
    
    """ + operation_per_cell + """

    logTraceOut( "touchCellFirstTime(...)" );
  } 
}
"""

    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = jinja2.Template(self.guard, undefined=jinja2.DebugUndefined).render(**d)
      result = jinja2.Template(self._compute_kernel, undefined=jinja2.DebugUndefined).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

