# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2
import jinja2


from .AbstractRKFDActionSet import AbstractRKFDActionSet


class DynamicAMR( peano4.toolbox.blockstructured.DynamicAMR):
  """
  
   The handling of (dynamically) adaptive meshes for finite differences
   
   This class just invokes interpolation operators. Please consult the documentation
   of the base class CellCenteredFiniteDifferences for a discussion of interpolation
   and restriction operators. The class documentation also explains how you switch 
   to operators that you pass in manually.
  
  """
    
    
  def __init__(self,
               solver,
               interpolation,
               restriction):
    """
     Construct object
    
    
     :: Interpolation
     
     Don't interpolate in initialisation. If you have a parallel run with AMR, then some 
     boundary data has not been received yet and your face data thus is not initialised 
     at all. Other than that, the interpolation is always on.


     :: Restriction
     
     In contrast to the interpolation, it is absolutely key that we restrict alreaday in
     the initialisation. If we have a cell that is adjacent to a refined cell, then this
     cell requires proper face data for the subsequent first time step. So we have to 
     restrict in the initialisation even if there's no update yet - just to get the 
     coarse representation of fine hanging faces initialised.
    
          
     :: Restriction of destructed faces
     
     A destructed face has to restrict its data: We've already restricted the cells,
     but now we also have to restrict the faces, as faces span time spans. The cell
     is only a snapshot, so we have to manually restrict.
     
     It is important to also disable the projection in return. The projection onto the
     faces happens in leaveCell. However, if you have a face in-between two 3x3 patches,
     then you will have a destruction of the left patch (and a corresponding cell restriction),
     the coarse cell will then project its data onto the face, you go down on the other
     side and now the face will be destroyed. If you restrict now, you overwrite the 
     projected data with some (old) data, and you will get an inconsistent face data.
     However, you always have to restrict both sides of persistent face when it is 
     destroyed. If you only restricted the inner part, you'd run into situations where
     a 3x3 patch is coarsened, but its neighbour remains refined. That is, logically the
     face transitions from a persistent one to a hanging face. When yo go through the 
     mesh next time, you'll need, on the coarse level, some valid data. So we have to 
     restrict both sides.
     
     There's still a inconsistency here. To have really consistent data, we'd have to 
     average 3^{d-1} cells when we project them onto the face, such that the cell-restricted
     data followed by a (single cell) projection gives the same result as a projection 
     first and then a face restriction. I neglect this fact here.
  
  
     :: Interpolation throughout the grid initialisation
     
     Throughout the grid initialisation, we cannot interpolate the halo layers reliably.
     If we descend into a cell and want to initialise one of its hanging faces, we cannot
     know, if the coarser cell on the other side has been initialised already. Therefore,
     we cannot initialise the halo of a hanging face.
     
     For persistent faces, this is something we don't bother about: We create both 
     adjacent cells of the respective face and then we will project this solution onto
     the faces. Problem set, as all data is properly set. 
     
     For hanging faces, we will get the projection from the adjacent real cell. But
     we cannot get a proper initialisation for the other half of the face data. These
     data however is required for some restrictions. Even if we mask out the impact
     of halo data on the restriction, it might, in the initialisation sweep, hold 
     nans, and 0 * nan = nan. So we will get garbage. In later time steps, this does
     not hold, as we have properly initialised coarse grid data then on all sides. 
     
     To solve this issue, we initialise the hanging face data properly using the user's
     callbacks. These data will later be overwritten with (coarser) data which is 
     interpolated. For the initialisation, it provides us with proper initial guesses. 
          
             
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
          ::exahype2::fd::getGridFaceCentre( marker.x(), marker.h(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fd::getGridFaceSize( marker.h(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace""" + solver._name + """QOld.value + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fd::getGridFaceCentre( marker.x(), marker.h(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fd::getGridFaceSize( marker.h(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace""" + solver._name + """QUpdate.value + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fd::getGridFaceCentre( marker.x(), marker.h(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fd::getGridFaceSize( marker.h(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
      }
    }
    
    logTraceOut( "createHangingFace(...)" );
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
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/fv/InterpolationRestriction.h"
"""    
