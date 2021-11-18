# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

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
    
    
  def __init__(self,solver):
    """
    
     # Interpolation
     
     Don't interpolate in initialisation. If you have a parallel run with AMR, then some 
     boundary data has not been received yet and your face data thus is not initialised 
     at all.
     
     # Restriction of destructed faces
     
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
          
    """
    peano4.toolbox.blockstructured.DynamicAMR.__init__(self,      
      patch = solver._patch,
      patch_overlap_interpolation = solver._patch_overlap_old,
      patch_overlap_restriction   = solver._patch_overlap_update,
      interpolate_guard           = """
  repositories::""" + solver.get_name_of_global_instance() + """.getSolverState()!=""" + solver._name + """::SolverState::GridInitialisation
""",
      restrict_guard           = """
  repositories::""" + solver.get_name_of_global_instance() + """.getSolverState()!=""" + solver._name + """::SolverState::GridInitialisation
""",
      additional_includes         = """
#include "../repositories/SolverRepository.h"
#include "exahype2/fv/Generic.h"
"""      
)
    self.__Template_DestroyHangingFace_Core += """
  bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= Dimensions;
  double newTimeStamp = std::max( coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), fineGridFace""" + solver._face_label.name + """.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ));
  coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
  coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1,newTimeStamp);
"""

    self.__Template_DestroyPersistentFace_Core += """ 
  // Copy over updated data. This is inaccurate, as we should restrict the updated
  // and the old value separatedly. But the restriction accumulates data in the 
  // coarse level, so we'd have to clear it beforehand. However, we cannot clear
  // it as we need for the BCs and an interpolation. So I accept that I introduce
  // an error here.
  #if Dimensions==2
  constexpr int NumberOfBytes = {{DOFS_PER_AXIS}}*2*{{OVERLAP}}*{{UNKNOWNS}}*sizeof(double);
  #else
  constexpr int NumberOfBytes = {{DOFS_PER_AXIS}}*{{DOFS_PER_AXIS}}*2*{{OVERLAP}}*{{UNKNOWNS}}*sizeof(double);
  #endif
  std::memcpy( 
    coarseGridFaces""" + solver._name + """QNew(marker.getSelectedFaceNumber()).value, 
    fineGridFace""" + solver._name + """QNew.value,
    NumberOfBytes
  );
  std::memcpy( 
    coarseGridFaces""" + solver._name + """QOld(marker.getSelectedFaceNumber()).value, 
    fineGridFace""" + solver._name + """QOld.value,
    NumberOfBytes
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
"""

    self.__Template_CreateHangingFace_Core  = """
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace""" + solver._name + """QOld.value,
      coarseGridFaces""" + solver._name + """QOld(marker.getSelectedFaceNumber()).value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace""" + solver._name + """QNew.value,
      coarseGridFaces""" + solver._name + """QNew(marker.getSelectedFaceNumber()).value
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
"""


    self.__Template_CreatePersistentFace_Core = """
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace""" + solver._name + """QOld.value,
      coarseGridFaces""" + solver._name + """QOld(marker.getSelectedFaceNumber()).value,
      coarseGridCell""" + solver._name + """Q.value
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace""" + solver._name + """QNew.value,
      coarseGridFaces""" + solver._name + """QNew(marker.getSelectedFaceNumber()).value,
      coarseGridCell""" + solver._name + """Q.value
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
    fineGridFace""" + solver._face_label.name + """.setNewTimeStamp(coarseGridCell""" + solver._cell_label.name + """.getTimeStamp());
    fineGridFace""" + solver._face_label.name + """.setOldTimeStamp(coarseGridCell""" + solver._cell_label.name + """.getTimeStamp());
"""

    self.__Template_CreateCell_Core += """    
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
"""

    self.__Template_DestroyCell_Core += """    
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
"""



    
  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(DynamicAMR,self).get_includes() + """
#include <cstring>
"""    
