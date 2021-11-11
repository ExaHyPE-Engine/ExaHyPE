# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
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
    #
    # Don't interpolate in initialisation. If you have a parallel run with AMR, then some 
    # boundary data has not been received yet and your face data thus is not initialised 
    # at all.
    #
  def __init__(self,solver):
    peano4.toolbox.blockstructured.DynamicAMR.__init__(self,      
      patch = solver._patch,
      patch_overlap_interpolation = solver._patch_overlap_old,
      patch_overlap_restriction   = solver._patch_overlap_update,
      interpolate_guard           = """
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
  // have to copy over manually. This stuff is not cleared, so there will be rubbish
  // in there, but I need it for the boundary data
  #if Dimensions==2
  for (int i=0; i<{{DOFS_PER_AXIS}}*2*{{OVERLAP}}; i++) {
  #else
  for (int i=0; i<{{DOFS_PER_AXIS}}*{{DOFS_PER_AXIS}}*2*{{OVERLAP}}; i++) {
  #endif
    for (int unknown=0; unknown<{{UNKNOWNS}}; unknown++) {
      coarseGridFaces""" + solver._name + """QOld(marker.getSelectedFaceNumber()).value[i*{{UNKNOWNS}}+unknown] = coarseGridFaces""" + solver._name + """QUpdate(marker.getSelectedFaceNumber()).value[i*{{UNKNOWNS}}+unknown];
      coarseGridFaces""" + solver._name + """QNew(marker.getSelectedFaceNumber()).value[i*{{UNKNOWNS}}+unknown] = coarseGridFaces""" + solver._name + """QUpdate(marker.getSelectedFaceNumber()).value[i*{{UNKNOWNS}}+unknown];
    }
  }

  double newTimeStamp = 0.0;
  newTimeStamp = std::max( newTimeStamp,
    coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp(0)
  ); 
  newTimeStamp = std::max( newTimeStamp,
    coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp(1)
  );
  newTimeStamp = std::max( newTimeStamp,
    fineGridFace""" + solver._face_label.name + """.getUpdatedTimeStamp(0)
  );
  newTimeStamp = std::max( newTimeStamp,
    fineGridFace""" + solver._face_label.name + """.getUpdatedTimeStamp(1)
  );
  coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdated(true);
  coarseGridFaces""" + solver._face_label.name + """(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(newTimeStamp);
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

