# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2
import jinja2

import peano4.toolbox.blockstructured.DynamicAMR


class DynamicAMR(peano4.toolbox.blockstructured.DynamicAMR):
    """
    The ExaHyPE 2 Finite Volume handling of (dynamically) adaptive meshes
    """

    def __init__(self, solver, interpolation, restriction):
        super(DynamicAMR, self).__init__(
            patch=solver._patch,
            patch_overlap_interpolation=solver._patch_overlap_old,
            patch_overlap_restriction=solver._patch_overlap_update,
            interpolation_scheme=interpolation,
            restriction_scheme=restriction,
            clear_guard=solver._provide_face_data_to_compute_kernels_default_guard(),
            interpolate_guard=solver._provide_face_data_to_compute_kernels_default_guard()
            + """ and 
  repositories::"""
            + solver.get_name_of_global_instance()
            + """.getSolverState()!="""
            + solver._name
            + """::SolverState::GridInitialisation
""",
            restrict_guard=solver._provide_face_data_to_compute_kernels_default_guard(),
            additional_includes="""
#include "../repositories/SolverRepository.h"
""",
        )

        self.d["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.d["FINE_GRID_FACE_ACCESSOR_INTERPOLATION"] = "fineGridFace" + solver._name
        self.d["FINE_GRID_FACE_ACCESSOR_INTERPOLATION"] = "fineGridFace" + solver._name
        self.d["COARSE_GRID_FACE_ACCESSOR_INTERPOLATION"] = (
            "coarseGridFaces" + solver._name
        )
        self.d["FINE_GRID_FACE_ACCESSOR_RESTRICTION"] = "fineGridFace" + solver._name
        self.d["COARSE_GRID_FACE_ACCESSOR_RESTRICTION"] = (
            "coarseGridFaces" + solver._name
        )

        self._Template_TouchFaceFirstTime = """
  if ( {{CLEAR_GUARD}} ) {
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate.data()
    );
  }
"""

        """
    Very similar to original toolbox's routine, but this one restricts
    into QUpdate, and leaves QOld and QNew unchanged. Furthermore to the
    actual data restriction, we also restrict the updated flag and the 
    time face's time stamp.
    """
        self._Template_TouchHangingFaceLastTime = (
            """
  if ({{RESTRICT_GUARD}}) {
    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate.data(),
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate(marker.getSelectedFaceNumber()).data()
    );

    bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= DIMENSIONS;
    double newTimeStamp = std::max( coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), fineGridFace"""
            + solver._face_label.name
            + """.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ));
    coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1,newTimeStamp);
  }
"""
        )

        self._Template_DestroyFace = (
            """
  if ( not marker.isInteriorFaceWithinPatch() ) {
    ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate.data(),
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QUpdate(marker.getSelectedFaceNumber()).data()
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QNew.data(),
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QNew(marker.getSelectedFaceNumber()).data()
    );

    ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}QOld.data(),
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}QOld(marker.getSelectedFaceNumber()).data()
    );

    // A coarse face might have been non-persistent before. So it might
    // not carry a valid boundary flag, and we have to re-analyse it and
    // set it accordingly.
    tarch::la::Vector<DIMENSIONS, double> offset(DomainOffset);
    tarch::la::Vector<DIMENSIONS, double> size(DomainSize);
    bool isBoundary = false;
    for (int d=0; d<DIMENSIONS; d++) {
      isBoundary |= tarch::la::equals( marker.getX()(d), offset(d) );
      isBoundary |= tarch::la::equals( marker.getX()(d), offset(d) + size(d) );
    }
    coarseGridFaces"""
            + exahype2.grid.UpdateFaceLabel.get_attribute_name(solver._name)
            + """(marker.getSelectedFaceNumber()).setBoundary( isBoundary );

    // left and right
    for (int i=0; i<2; i++) {
      double newTimeStamp     = std::max( coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).getNewTimeStamp(     i ), fineGridFace"""
            + solver._face_label.name
            + """.getNewTimeStamp( i ));
      double oldTimeStamp     = std::max( coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).getOldTimeStamp(     i ), fineGridFace"""
            + solver._face_label.name
            + """.getOldTimeStamp( i ));
      double updatedTimeStamp = std::max( coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( i ), fineGridFace"""
            + solver._face_label.name
            + """.getUpdatedTimeStamp( i ));
      coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).setUpdated(         i,true);
      coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).setNewTimeStamp(    i,newTimeStamp);
      coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).setOldTimeStamp(    i,oldTimeStamp);
      coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(i,updatedTimeStamp);
    }
  }
"""
        )

        """
    Again, a 1:1 extension of the toolbox routine which interpolates both QOld
    and QNew. The clue here is that we treat the solution initialisation 
    separately: Here, we do not interpolate. We initialise using the solver's
    callback themselves.
    """
        self._Template_TouchHangingFaceFirstTime = (
            """
  if ( {{INTERPOLATE_GUARD}} ) {
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}}QNew(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}QNew.data()
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridFaces"""
            + solver._name
            + """QOld(marker.getSelectedFaceNumber()).data(),
      fineGridFace"""
            + solver._name
            + """QOld.data()
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridFaces"""
            + solver._name
            + """QNew(marker.getSelectedFaceNumber()).data(),
      fineGridFace"""
            + solver._name
            + """QNew.data()
    );

    // It is important that we clear the halo layer. If we have two layers of 
    // AMR, the finest one will restrict into QUpdate (so it has to be properly
    // initialised as 0).
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace"""
            + solver._name
            + """QUpdate.data()
    );
    const int leftRightEntry = marker.getSelectedFaceNumber()<DIMENSIONS ? 0 : 1;
    fineGridFace"""
            + solver._face_label.name
            + """.setNewTimeStamp(leftRightEntry,coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).getNewTimeStamp(leftRightEntry));
    fineGridFace"""
            + solver._face_label.name
            + """.setOldTimeStamp(leftRightEntry,coarseGridFaces"""
            + solver._face_label.name
            + """(marker.getSelectedFaceNumber()).getOldTimeStamp(leftRightEntry));
  }
  else if ( repositories::"""
            + solver.get_name_of_global_instance()
            + """.getSolverState()=="""
            + solver._name
            + """::SolverState::GridInitialisation ) {
    /*
     @todo Not required for FV, but compare to FD4 variant what has to be done throughout initialisation.
    int normal = marker.getSelectedFaceNumber() % DIMENSIONS;
    dfore(i, {{DOFS_PER_AXIS}}, normal, 0) {
      for( int j=0; j<2*{{OVERLAP}}; j++) {
        tarch::la::Vector<DIMENSIONS, int> dof = i;
        dof(normal) = j;

        int serialisedDoF = ::toolbox::blockstructured::serialiseVoxelIndexInOverlap(
          dof,
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal
        );

        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace"""
            + solver._name
            + """QNew.data() + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fv::getVolumeCentre( marker.getX(), marker.getH(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.getH(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace"""
            + solver._name
            + """QOld.data() + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fv::getVolumeCentre( marker.getX(), marker.getH(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.getH(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
        repositories::{{SOLVER_INSTANCE}}.initialCondition(
          fineGridFace"""
            + solver._name
            + """QUpdate.data() + serialisedDoF * {{UNKNOWNS}},
          ::exahype2::fv::getVolumeCentre( marker.getX(), marker.getH(), {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal, dof), 
          ::exahype2::fv::getVolumeSize( marker.getH(), {{DOFS_PER_AXIS}} ),
          true // grid grid is constructed
        );
      }
    }
    */
  }
"""
        )

        """
    Very similar to default version, but this time we interpolate QOld and
    QNew. We also clear the update field via clearHaloLayerAoS(). It is
    important that we initialise the time step sizes properly. As we deal
    with a persistent face, we set both the left and right time stamp from
    the coarse face.
    """
        self._Template_CreateFace = (
            """
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridCell"""
            + solver._name
            + """Q.data(),
      coarseGridFaces"""
            + solver._name
            + """QOld(marker.getSelectedFaceNumber()).data(),
      fineGridFace"""
            + solver._name
            + """QOld.data()
  );
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      coarseGridCell"""
            + solver._name
            + """Q.data(),
      coarseGridFaces"""
            + solver._name
            + """QNew(marker.getSelectedFaceNumber()).data(),
      fineGridFace"""
            + solver._name
            + """QNew.data()
  );

  // It is important that we clear the halo layer. If we have two layers of 
  // AMR, the finest one will restrict into QUpdate (so it has to be properly
  // initialised as 0).
  ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      fineGridFace"""
            + solver._name
            + """QUpdate.data()
  );
  fineGridFace"""
            + solver._face_label.name
            + """.setNewTimeStamp(coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp());
  fineGridFace"""
            + solver._face_label.name
            + """.setOldTimeStamp(coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp());
"""
        )

        self._Template_CreateCell = (
            """
  ::toolbox::blockstructured::interpolateCell_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{COARSE_GRID_CELL}}.data(),
      {{FINE_GRID_CELL}}.data()
  );

  #if PEANO_DEBUG
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  #endif

  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp( coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp() );
  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize( coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize() );
"""
        )

        self._Template_DestroyCell = (
            """
  ::toolbox::blockstructured::restrictCell_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{FINE_GRID_CELL}}.data(),
      {{COARSE_GRID_CELL}}.data()
  );

  #if PEANO_DEBUG
  // Exploit the fact that we can "reuse" some FV utils here
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  #endif

  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp( fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp() );
  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize( fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize() );
"""
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(DynamicAMR, self).get_includes()
            + """
#include <cstring>
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/InterpolationRestriction.h"
"""
        )
