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

        face_label_name = solver._face_label.name
        cell_label_name = solver._cell_label.name
        update_face_label_attribute = exahype2.grid.UpdateFaceLabel.get_attribute_name(solver._name)
        default_guard = solver._provide_face_data_to_compute_kernels_default_guard()
        super(DynamicAMR, self).__init__(
            patch=solver._patch,
            patch_overlap_interpolation=solver._patch_overlap_new,
            patch_overlap_restriction=solver._patch_overlap_new,
            interpolation_scheme=interpolation,
            restriction_scheme=restriction,
            clear_guard=default_guard,
            interpolate_guard=(
                f"{default_guard} and \n"
                f" repositories::{solver.get_name_of_global_instance()}.getSolverState()!={solver._name}::SolverState::GridInitialisation\n"
            ),
            restrict_guard=default_guard,
            additional_includes="""
#include "../repositories/SolverRepository.h"
""",
        )

        # Solver identity
        self.d["SOLVER_NAME"]       = solver._name
        self.d["SOLVER_REPOSITORY"] = f"repositories::{solver.get_name_of_global_instance()}"

        self.d["FINE_GRID_CELL_LABEL_ACCESSOR"]   = f"fineGridCell{cell_label_name}"
        self.d["COARSE_GRID_CELL_LABEL_ACCESSOR"] = f"coarseGridCell{cell_label_name}"
        self.d["FINE_GRID_CELL_Q_NEW_ACCESSOR"]   = f"fineGridCell{solver.current_unknown_identifier()}"
        self.d["FINE_GRID_CELL_Q_OLD_ACCESSOR"]   = f"fineGridCell{solver.previous_unknown_identifier()}"
        self.d["COARSE_GRID_CELL_Q_NEW_ACCESSOR"] = f"coarseGridCell{solver.current_unknown_identifier()}"
        self.d["COARSE_GRID_CELL_Q_OLD_ACCESSOR"] = f"coarseGridCell{solver.previous_unknown_identifier()}"

        self.d["FINE_GRID_FACE_LABEL_ACCESSOR"]           = f"fineGridFace{face_label_name}"
        self.d["COARSE_GRID_FACES_LABEL_ACCESSOR"]        = f"coarseGridFaces{face_label_name}"
        self.d["COARSE_GRID_FACES_UPDATE_LABEL_ACCESSOR"] = f"coarseGridFaces{update_face_label_attribute}"

        self.d["FINE_GRID_FACE_Q_NEW_ACCESSOR"]    = f"fineGridFace{solver.current_unknown_identifier()}"
        self.d["FINE_GRID_FACE_Q_OLD_ACCESSOR"]    = f"fineGridFace{solver.previous_unknown_identifier()}"

        self.d["COARSE_GRID_FACES_Q_NEW_ACCESSOR"]    = f"coarseGridFaces{solver.current_unknown_identifier()}"
        self.d["COARSE_GRID_FACES_Q_OLD_ACCESSOR"]    = f"coarseGridFaces{solver.previous_unknown_identifier()}"

        self._Template_TouchFaceFirstTime = """
"""

        # Restricts the inner half of the halo layer from fine to coarse. In
        # addition to the actual data restriction, we also restrict the updated
        # flag and the face's time stamp.
        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data()
    );
    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
      {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data()
    );

    bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= DIMENSIONS;
    double newTimeStamp = std::max( {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ));
    {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1,newTimeStamp);
  }
"""

        self._Template_DestroyFace = """
  if ( not marker.isInteriorFaceWithinPatch() ) {
    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data()
    );
    ::toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
      {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data()
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
    {{COARSE_GRID_FACES_UPDATE_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setBoundary( isBoundary );

    // left and right
    for (int i=0; i<2; i++) {
      double newTimeStamp     = std::max( {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getNewTimeStamp(     i ), {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getNewTimeStamp( i ));
      double oldTimeStamp     = std::max( {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getOldTimeStamp(     i ), {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getOldTimeStamp( i ));
      double updatedTimeStamp = std::max( {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( i ), {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdatedTimeStamp( i ));
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdated(         i,true);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setNewTimeStamp(    i,newTimeStamp);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setOldTimeStamp(    i,oldTimeStamp);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(i,updatedTimeStamp);
    }
  }
"""

        # A 1:1 extension of the toolbox routine which interpolates both
        # QOld and QNew. The clue here is that we treat the solution
        # initialisation separately: Here, we do not interpolate. We initialise
        # using the solver's callback themselves.
        self._Template_TouchHangingFaceFirstTime = """
  if ( {{INTERPOLATE_GUARD}} ) {
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data()
    );
    ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data()
    );

    const int leftRightEntry = marker.getSelectedFaceNumber()<DIMENSIONS ? 0 : 1;
    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setNewTimeStamp(leftRightEntry,{{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getNewTimeStamp(leftRightEntry));
    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setOldTimeStamp(leftRightEntry,{{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getOldTimeStamp(leftRightEntry));
  }
"""

        # Very similar to default version, but this time we interpolate QOld
        # and QNew. It is important that we initialise the time step sizes
        # properly. As we deal with a persistent face, we set both the left
        # and right time stamp from the coarse face.
        self._Template_CreateFace = """
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data()
  );
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data()
  );

  {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setNewTimeStamp({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
  {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setOldTimeStamp({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
"""

        self._Template_CreateCell = """
  ::toolbox::blockstructured::interpolateCell_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data()
  );
  ::toolbox::blockstructured::interpolateCell_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data()
  );

  #if PEANO_DEBUG
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  #endif

  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp( {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp() );
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize( {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize() );
"""

        self._Template_DestroyCell = """
  ::toolbox::blockstructured::restrictCell_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data()
  );
  ::toolbox::blockstructured::restrictCell_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data()
  );

  #if PEANO_DEBUG
  // Exploit the fact that we can "reuse" some FV utils here
  ::exahype2::fv::validatePatch(
    {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  ::exahype2::fv::validatePatch(
    {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  #endif

  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp( {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp() );
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize( {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize() );
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return f"""{super(DynamicAMR, self).get_includes()}
#include <cstring>
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/InterpolationRestriction.h"
"""
