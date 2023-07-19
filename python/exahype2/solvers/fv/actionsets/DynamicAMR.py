# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2
import jinja2


import peano4.toolbox.blockstructured.DynamicAMR                 


class DynamicAMR( peano4.toolbox.blockstructured.DynamicAMR ):
  """
  
   The ExaHyPE 2 Finite Volume handling of (dynamically) adaptive meshes
   
   This class is basically a plain copy of the DynamicAMR action set
   from the toolbox. However, we have to ensure that we also set the
   update markers appropriately such that restricted data are taken 
   into account. This action set thus has to be studied in combination 
   with the ProjectPatchOntoFaces action set which is very similar and
   also enriches the toolbox version by this marker aspect.
  
  """
    
    
  def __init__(self,
               solver,
               interpolation,
               restriction):
    """
     Construct object
    
     Please consult exahype2.solvers.rkfd.actionsets.DynamicAMR's class 
     documentation of details.
          
    """
    super( DynamicAMR, self ).__init__(    
      patch = solver._patch,
      patch_overlap_interpolation = solver._patch_overlap_old,
      patch_overlap_restriction   = solver._patch_overlap_update,
      interpolation_scheme        = interpolation,
      restriction_scheme          = restriction,
      clear_guard                 = solver._provide_face_data_to_compute_kernels_default_guard(),
      interpolate_guard           = solver._provide_face_data_to_compute_kernels_default_guard() + """ and 
  repositories::""" + solver.get_name_of_global_instance() + """.getSolverState()!=""" + solver._name + """::SolverState::GridInitialisation
""",
      restrict_guard              = solver._provide_face_data_to_compute_kernels_default_guard(),
      additional_includes         = """
#include "../repositories/SolverRepository.h"
"""      
)

    self.d[ "SOLVER_INSTANCE" ]                         = solver.get_name_of_global_instance()
    self.d[ "FINE_GRID_FACE_ACCESSOR_INTERPOLATION" ]   = "fineGridFace" + solver._name
    self.d[ "FINE_GRID_FACE_ACCESSOR_INTERPOLATION" ]   = "fineGridFace" + solver._name
    self.d[ "COARSE_GRID_FACE_ACCESSOR_INTERPOLATION" ] = "coarseGridFaces" + solver._name
    self.d[ "FINE_GRID_FACE_ACCESSOR_RESTRICTION" ]     = "fineGridFace" + solver._name
    self.d[ "COARSE_GRID_FACE_ACCESSOR_RESTRICTION" ]   = "coarseGridFaces" + solver._name

    """
    
    Touch first time does nothing proper. It simply clears the halo
    layer.
    
    """
    self._Template_TouchFaceFirstTime = """
  if ( {{CLEAR_GUARD}} ) {
    logTraceInWith2Arguments( "touchFaceFirstTime(...)", marker.toString(), "clear halo layer {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}" );
    
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate.value
    );

    logTraceOut( "touchFaceFirstTime(...)" );
  }
"""

    """
    
    Very similar to original toolbox's routine, but this one restricts
    into QUpdate, and leaves QOld and QNew unchanged. Furthermore to the
    actual data restriction, we also restrict the updated flag and the 
    time face's time stamp.
    
    """
    self._Template_DestroyHangingFace = """
  if ( {{RESTRICT_GUARD}} ) {
    logTraceInWith4Arguments( "destroyHangingFace(...)", "{{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}", "{{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}", marker.getSelectedFaceNumber(), marker.toString() );

    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate.value,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate(marker.getSelectedFaceNumber()).value
    );

    bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= Dimensions;
    double newTimeStamp = std::max( coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), fineGridFace""" + solver._face_label.name + """.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ));
    coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1,newTimeStamp);
    
    logTraceOut( "destroyHangingFace(...)" );
  }
"""

    self._Template_DestroyPersistentFace = """ 
  if ( not marker.isInteriorFaceWithinPatch() ) {
    logTraceIn( "destroyPersistentFace(...)" );
    
    ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate.value,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QNew.value,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QNew(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QOld.value,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QOld(marker.getSelectedFaceNumber()).value
    );
    
    // A coarse face might have been non-persistent before. So it might
    // not carry a valid boundary flag, and we have to re-analyse it and
    // set it accordingly.    
    tarch::la::Vector<Dimensions, double> offset(DomainOffset);
    tarch::la::Vector<Dimensions, double> size(DomainSize);
    bool isBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
      isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
    }
    coarseGridFaces""" + exahype2.grid.UpdateFaceLabel.get_attribute_name(solver._name) + """(marker.getSelectedFaceNumber()).setBoundary( isBoundary );

    // left and right
    for (int i=0; i<2; i++) {
      double newTimeStamp     = std::max( coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getNewTimeStamp(     i ), fineGridFace""" + solver._face_label.name + """.getNewTimeStamp( i ));
      double oldTimeStamp     = std::max( coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getOldTimeStamp(     i ), fineGridFace""" + solver._face_label.name + """.getOldTimeStamp( i ));
      double updatedTimeStamp = std::max( coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( i ), fineGridFace""" + solver._face_label.name + """.getUpdatedTimeStamp( i ));
      coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdated(         i,true);
      coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setNewTimeStamp(    i,newTimeStamp);
      coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setOldTimeStamp(    i,oldTimeStamp);
      coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(i,updatedTimeStamp);
    }
  
    logTraceOut( "destroyPersistentFace(...)" );
  }
"""

    """
    
    Again, a 1:1 extension of the toolbox routine which interpolates both QOld
    and QNew. The clue here is that we treat the solution initialisation 
    separately: Here, we do not interpolate. We initialise using the solver's
    callback themselves.
    
    """
    self._Template_CreateHangingFace = """
  if ( {{INTERPOLATE_GUARD}} ) {
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );

    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}}QNew(marker.getSelectedFaceNumber()).value,
      {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}QNew.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridFaces""" + solver._name + """QOld(marker.getSelectedFaceNumber()).value,
      fineGridFace""" + solver._name + """QOld.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridFaces""" + solver._name + """QNew(marker.getSelectedFaceNumber()).value,
      fineGridFace""" + solver._name + """QNew.value
    );

    // It is important that we clear the halo layer. If we have two layers of 
    // AMR, the finest one will restrict into QUpdate (so it has to be properly
    // initialised as 0).
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace""" + solver._name + """QUpdate.value
    );
    const int leftRightEntry = marker.getSelectedFaceNumber()<Dimensions ? 0 : 1;
    fineGridFace""" + solver._face_label.name + """.setNewTimeStamp(leftRightEntry,coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getNewTimeStamp(leftRightEntry));
    fineGridFace""" + solver._face_label.name + """.setOldTimeStamp(leftRightEntry,coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getOldTimeStamp(leftRightEntry));

    logTraceOut( "createHangingFace(...)" );
  }
  else if ( repositories::""" + solver.get_name_of_global_instance() + """.getSolverState()==""" + solver._name + """::SolverState::GridInitialisation ) {
    /*
    
     @todo Not required for FV, but compare to FD4 variant what has to be done 
           throughout initialisation.
     
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );
    
    int normal = marker.getSelectedFaceNumber() % Dimensions;
    dfore(i, {{DOFS_PER_AXIS}}, normal, 0) {
      for( int j=0; j<2*{{OVERLAP}}; j++) {
        tarch::la::Vector<Dimensions, int> dof = i;
        dof(normal) = j;
        
        int serialisedDoF = ::toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          dof,
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal
        );
        
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace""" + solver._name + """QNew.value + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.h(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace""" + solver._name + """QOld.value + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.h(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace""" + solver._name + """QUpdate.value + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.h(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
      }
    }
    
    logTraceOut( "createHangingFace(...)" );
    */
  }
"""

    """
  
    Very similar to default version, but this time we interpolate QOld and
    QNew. We also clear the update field via clearHaloLayerAoS(). It is 
    important that we initialise the time step sizes properly. As we deal
    with a persistent face, we set both the left and right time stamp from
    the coarse face.
  
    """
    self._Template_CreatePersistentFace = """
  logTraceInWith1Argument( "createPersistentFace(...)", marker.toString() );
  
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridCell""" + solver._name + """Q.value,
      coarseGridFaces""" + solver._name + """QOld(marker.getSelectedFaceNumber()).value,
      fineGridFace""" + solver._name + """QOld.value
  );
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridCell""" + solver._name + """Q.value,
      coarseGridFaces""" + solver._name + """QNew(marker.getSelectedFaceNumber()).value,
      fineGridFace""" + solver._name + """QNew.value
  );
  
  // It is important that we clear the halo layer. If we have two layers of 
  // AMR, the finest one will restrict into QUpdate (so it has to be properly
  // initialised as 0).
  ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace""" + solver._name + """QUpdate.value
  );
  fineGridFace""" + solver._face_label.name + """.setNewTimeStamp(coarseGridCell""" + solver._cell_label.name + """.getTimeStamp());
  fineGridFace""" + solver._face_label.name + """.setOldTimeStamp(coarseGridCell""" + solver._cell_label.name + """.getTimeStamp());
  
  logTraceOutWith1Argument( "createPersistentFace(...)", marker.toString() );
"""

    self._Template_CreateCell = """    
  logTraceIn( "createCell(...)" );

  ::toolbox::blockstructured::interpolateCell_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{COARSE_GRID_CELL}}.value,
      {{FINE_GRID_CELL}}.value
  );

  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL}}.value,
    {{UNKNOWNS}},
    0, // auxiliary values. Not known here
    {{DOFS_PER_AXIS}},
    0, // no halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); 

  fineGridCell""" + solver._cell_label.name + """.setTimeStamp( coarseGridCell""" + solver._cell_label.name + """.getTimeStamp() );
  fineGridCell""" + solver._cell_label.name + """.setTimeStepSize( coarseGridCell""" + solver._cell_label.name + """.getTimeStepSize() );

  logTraceOut( "createCell(...)" );
"""

    self._Template_DestroyCell = """
  logTraceInWith1Argument( "destroyCell(...)", marker.toString() );

  ::toolbox::blockstructured::restrictCell_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{FINE_GRID_CELL}}.value,
      {{COARSE_GRID_CELL}}.value
  );

  // exploit the fact that we can "reuse" some FV utils here    
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL}}.value,
    {{UNKNOWNS}},
    0, // auxiliary values. Not known here
    {{DOFS_PER_AXIS}},
    0, // no halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); 
  
  coarseGridCell""" + solver._cell_label.name + """.setTimeStamp( fineGridCell""" + solver._cell_label.name + """.getTimeStamp() );
  coarseGridCell""" + solver._cell_label.name + """.setTimeStepSize( fineGridCell""" + solver._cell_label.name + """.getTimeStepSize() );
  
  logTraceOut( "destroyCell(...)" );
"""

    
  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(DynamicAMR,self).get_includes() + """
#include <cstring>
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/InterpolationRestriction.h"
"""    
