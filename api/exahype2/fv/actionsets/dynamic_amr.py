# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets


class AdaptiveMeshFaceCoupling(exahype2.actionsets.SolverActionSet):
    """Couples a hanging face to the coarse face it hangs below.

    The interpolation fills the halo a hanging face takes from the coarse face, and the
    restriction gathers the halo layers of the subfaces onto their coarse face.
    """

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  if ({{CLEAR_GUARD}}) {
    ::exahype2::fv::clearFacePatch<{{DOFS_PER_AXIS}}, {{OVERLAP}}, {{UNKNOWNS}}>({{FINE_GRID_FACE_Q_NEW}}.data());
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = """
  if ({{RESTRICT_GUARD}}) {
    ::exahype2::fv::clearFacePatch<{{DOFS_PER_AXIS}}, {{OVERLAP}}, {{UNKNOWNS}}>({{FINE_GRID_FACE_Q_NEW}}.data());
  }
  if ({{INTERPOLATE_GUARD}}) {
    ::exahype2::fv::interpolateHangingFace<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
      marker,
      {{COARSE_GRID_FACES_Q_OLD}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_OLD}}.data(),
      {{COARSE_GRID_FACES_LABEL}}(marker.getSelectedFaceNumber()),
      {{FINE_GRID_FACE_LABEL}}
    );
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
  if ({{RESTRICT_GUARD}}) {
    ::exahype2::fv::restrictHangingFace<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
      marker,
      _haloRestrictionStageQNew,
      {{COARSE_GRID_FACES_Q_NEW}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_NEW}}.data(),
      {{COARSE_GRID_FACES_LABEL}}(marker.getSelectedFaceNumber()),
      {{FINE_GRID_FACE_LABEL}}
    );
  }
"""

    _TEMPLATE_TOUCH_FACE_LAST_TIME = """
  if ({{RESTRICT_GUARD}}) {
    ::exahype2::fv::restrictFaceHalves<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
      marker,
      _haloRestrictionStageQNew,
      {{FINE_GRID_FACE_Q_NEW}}.data()
    );
  }
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = ""
    _TEMPLATE_TOUCH_CELL_LAST_TIME = ""
    _TEMPLATE_CREATE_FACE = ""
    _TEMPLATE_DESTROY_FACE = ""
    _TEMPLATE_CREATE_CELL = ""
    _TEMPLATE_DESTROY_CELL = ""

    def __init__(self, solver):
        super().__init__(solver, descend_invocation_order=1)

        patch_overlap = solver._patch_overlap_new
        if patch_overlap.dim[0] % 2 != 0:
            raise ValueError(
                "The patch of a face needs an even number of cells, so that its two "
                "overlap halves have the same width."
            )

        face_data_transfer_guard = solver._store_face_data_default_guard()
        self._entries = {
            "UNKNOWNS": patch_overlap.no_of_unknowns,
            "DOFS_PER_AXIS": patch_overlap.dim[1],
            "OVERLAP": int(patch_overlap.dim[0] / 2),
            "CLEAR_GUARD": face_data_transfer_guard,
            "RESTRICT_GUARD": face_data_transfer_guard,
            "INTERPOLATE_GUARD": (
                f"{face_data_transfer_guard} and "
                f"not repositories::{solver.instance_name}.isGridInitialisationSweep()"
            ),
            "PROJECT_GUARD": solver._store_cell_data_default_guard(),
            "FINE_GRID_CELL_LABEL": f"fineGridCell{solver._cell_label.name}",
            "COARSE_GRID_CELL_LABEL": f"coarseGridCell{solver._cell_label.name}",
            "FINE_GRID_CELL_Q_NEW": f"fineGridCell{solver.current_unknown_identifier()}",
            "FINE_GRID_CELL_Q_OLD": f"fineGridCell{solver.previous_unknown_identifier()}",
            "COARSE_GRID_CELL_Q_NEW": f"coarseGridCell{solver.current_unknown_identifier()}",
            "COARSE_GRID_CELL_Q_OLD": f"coarseGridCell{solver.previous_unknown_identifier()}",
            "FINE_GRID_FACE_LABEL": f"fineGridFace{solver._face_label.name}",
            "FINE_GRID_FACES_LABEL": f"fineGridFaces{solver._face_label.name}",
            "COARSE_GRID_FACES_LABEL": f"coarseGridFaces{solver._face_label.name}",
            "FINE_GRID_FACE_Q_NEW": f"fineGridFace{solver.current_unknown_identifier()}",
            "FINE_GRID_FACE_Q_OLD": f"fineGridFace{solver.previous_unknown_identifier()}",
            "FINE_GRID_FACES_Q_NEW": f"fineGridFaces{solver.current_unknown_identifier()}",
            "COARSE_GRID_FACES_Q_NEW": f"coarseGridFaces{solver.current_unknown_identifier()}",
            "COARSE_GRID_FACES_Q_OLD": f"coarseGridFaces{solver.previous_unknown_identifier()}",
        }

    @property
    def name(self):
        return "exahype2_fv_actionsets_" + type(self).__name__

    def _template_of_operation(self, operation_name):
        operations = exahype2.actionsets.ActionSet
        return {
            operations.OPERATION_CREATE_FACE: self._TEMPLATE_CREATE_FACE,
            operations.OPERATION_DESTROY_FACE: self._TEMPLATE_DESTROY_FACE,
            operations.OPERATION_CREATE_CELL: self._TEMPLATE_CREATE_CELL,
            operations.OPERATION_DESTROY_CELL: self._TEMPLATE_DESTROY_CELL,
            operations.OPERATION_TOUCH_FACE_FIRST_TIME: self._TEMPLATE_TOUCH_FACE_FIRST_TIME,
            operations.OPERATION_TOUCH_FACE_LAST_TIME: self._TEMPLATE_TOUCH_FACE_LAST_TIME,
            operations.OPERATION_TOUCH_CELL_FIRST_TIME: self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
            operations.OPERATION_TOUCH_CELL_LAST_TIME: self._TEMPLATE_TOUCH_CELL_LAST_TIME,
            operations.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME: self._TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME,
            operations.OPERATION_TOUCH_HANGING_FACE_LAST_TIME: self._TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME,
        }.get(operation_name)

    def operation_body(self, operation_name):
        template = self._template_of_operation(operation_name)
        if template is None:
            return "\n"
        return self.render(template, **self._entries)

    def attributes(self):
        return self.render(
            """
  ::exahype2::amr::ChildDataStage<{{STORAGE_PRECISION}}> _haloRestrictionStageQNew{
    ::exahype2::amr::SubfacesPerFace,
    ::exahype2::fv::ValuesPerFaceLayer<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>
  };
""",
            **self._entries,
        )

    def includes(self):
        return super().includes() + ["exahype2/fv/DynamicAMR.h"]


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """Moves the solution across a level jump the mesh refinement has created."""

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  ::exahype2::fv::clearCoarseningFace<{{DOFS_PER_AXIS}}, {{OVERLAP}}, {{UNKNOWNS}}>(
    marker,
    {{FINE_GRID_FACE_LABEL}},
    {{FINE_GRID_FACE_Q_NEW}}.data(),
    {{FINE_GRID_FACE_Q_OLD}}.data()
  );
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  ::exahype2::fv::clearCoarseningCell<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
    marker,
    {{FINE_GRID_CELL_Q_NEW}}.data(),
    {{FINE_GRID_CELL_Q_OLD}}.data()
  );
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = ""
    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = ""

    _TEMPLATE_TOUCH_FACE_LAST_TIME = """
  ::exahype2::fv::restrictCoarseningFaceHalves<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
    marker,
    _haloRestrictionStageQNew,
    _haloRestrictionStageQOld,
    {{FINE_GRID_FACE_Q_NEW}}.data(),
    {{FINE_GRID_FACE_Q_OLD}}.data(),
    {{FINE_GRID_FACE_LABEL}}
  );
"""

    _TEMPLATE_DESTROY_FACE = """
  ::exahype2::fv::destroyFace<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
    marker,
    _haloRestrictionStageQNew,
    _haloRestrictionStageQOld,
    {{COARSE_GRID_FACES_Q_NEW}}(marker.getSelectedFaceNumber()).data(),
    {{COARSE_GRID_FACES_Q_OLD}}(marker.getSelectedFaceNumber()).data(),
    {{FINE_GRID_FACE_Q_NEW}}.data(),
    {{FINE_GRID_FACE_Q_OLD}}.data(),
    {{COARSE_GRID_FACES_LABEL}}(marker.getSelectedFaceNumber()),
    {{FINE_GRID_FACE_LABEL}},
    DomainOffset,
    DomainSize
  );
"""

    _TEMPLATE_CREATE_FACE = """
  ::exahype2::fv::createFace<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
    marker,
    {{COARSE_GRID_CELL_Q_OLD}}.data(),
    {{COARSE_GRID_CELL_Q_NEW}}.data(),
    {{COARSE_GRID_FACES_Q_OLD}}(marker.getSelectedFaceNumber()).data(),
    {{COARSE_GRID_FACES_Q_NEW}}(marker.getSelectedFaceNumber()).data(),
    {{FINE_GRID_FACE_Q_OLD}}.data(),
    {{FINE_GRID_FACE_Q_NEW}}.data(),
    {{COARSE_GRID_CELL_LABEL}},
    {{FINE_GRID_FACE_LABEL}}
  );
"""

    _TEMPLATE_CREATE_CELL = """
  ::exahype2::fv::createCell<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
    marker,
    {{COARSE_GRID_CELL_Q_NEW}}.data(),
    {{FINE_GRID_CELL_Q_NEW}}.data(),
    {{COARSE_GRID_CELL_Q_OLD}}.data(),
    {{FINE_GRID_CELL_Q_OLD}}.data(),
    {{COARSE_GRID_CELL_LABEL}},
    {{FINE_GRID_CELL_LABEL}}
  );
"""

    _TEMPLATE_DESTROY_CELL = """
  ::exahype2::fv::destroyCell<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>(
    marker,
    _cellRestrictionStageQNew,
    _cellRestrictionStageQOld,
    {{COARSE_GRID_CELL_Q_NEW}}.data(),
    {{FINE_GRID_CELL_Q_NEW}}.data(),
    {{COARSE_GRID_CELL_Q_OLD}}.data(),
    {{FINE_GRID_CELL_Q_OLD}}.data(),
    {{COARSE_GRID_CELL_LABEL}},
    {{FINE_GRID_CELL_LABEL}}
  );
"""

    _TEMPLATE_TOUCH_CELL_LAST_TIME = """
  if ({{PROJECT_GUARD}}) {
    ::exahype2::fv::projectPatchAfterTransfer<{{DOFS_PER_AXIS}}, {{OVERLAP}}, {{UNKNOWNS}}>(
      marker,
      {{FINE_GRID_CELL_Q_NEW}}.data(),
      {{FINE_GRID_FACES_Q_NEW}},
      {{FINE_GRID_FACES_LABEL}},
      {{FINE_GRID_CELL_LABEL}}
    );
  }
"""

    def grid_control_events_body(self):
        return "  return ::exahype2::amr::gridControlEvents();\n"

    def attributes(self):
        return super().attributes() + self.render(
            """
  ::exahype2::amr::ChildDataStage<{{STORAGE_PRECISION}}> _haloRestrictionStageQOld{
    ::exahype2::amr::SubfacesPerFace,
    ::exahype2::fv::ValuesPerFaceLayer<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>
  };
  ::exahype2::amr::ChildDataStage<{{STORAGE_PRECISION}}> _cellRestrictionStageQNew{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    ::exahype2::fv::ValuesPerCellPatch<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>
  };
  ::exahype2::amr::ChildDataStage<{{STORAGE_PRECISION}}> _cellRestrictionStageQOld{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    ::exahype2::fv::ValuesPerCellPatch<{{DOFS_PER_AXIS}}, {{UNKNOWNS}}>
  };
""",
            **self._entries,
        )
