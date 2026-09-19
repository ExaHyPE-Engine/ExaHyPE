# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

import exahype2.datamodel

from .action_set import ActionSet


class UpdateFaceLabel(ActionSet):
    """
    A hanging face keeps RequiresDataTransfer between two traversals and no other label
    field, so the first touch of a hanging face sets every other field again. AboveHanging
    states per side whether finer cells hang below the face, and NewAboveHanging carries
    that state while the traversal ascends.
    """

    def get_attribute_name(solver_name):
        return solver_name + "FaceLabel"

    def __init__(self, solver_name):
        super(UpdateFaceLabel, self).__init__(descend_invocation_order=1)
        self._solver_name = solver_name

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  fineGridFace{{FACE_LABEL_NAME}}.setUpdated(false);
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging(false);
"""

    _TEMPLATE_CREATE_FACE = """
  const auto& offset     = DomainOffset;
  const auto& size       = DomainSize;
  bool        isBoundary = false;
  for (int d = 0; d < DIMENSIONS; d++) {
    isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d));
    isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d) + size(d));
  }
  fineGridFace{{FACE_LABEL_NAME}}.setBoundary(isBoundary);
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging(false);
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging(false);
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging(false);
  fineGridFace{{FACE_LABEL_NAME}}.setRequiresDataTransfer(0, true);
  fineGridFace{{FACE_LABEL_NAME}}.setRequiresDataTransfer(1, true);
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp(0.0);
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp(0.0);
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp(0.0);
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = """
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp(0.0);
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp(0.0);
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp(0.0);
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging(true);
  // Reused face storage can contain flags from a persistent face.
  // AboveHanging determines which half of the flux the cell reads.
  fineGridFace{{FACE_LABEL_NAME}}.setUpdated(false);
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging(false);
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging(false);
  // A hanging face never passes through createFace, so the boundary flag is set here with
  // the geometric test createFace uses. marker.getX() is the centre of the hanging face.
  {
    const auto& offset     = DomainOffset;
    const auto& size       = DomainSize;
    bool        isBoundary = false;
    for (int d = 0; d < DIMENSIONS; d++) {
      isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d));
      isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d) + size(d));
    }
    fineGridFace{{FACE_LABEL_NAME}}.setBoundary(isBoundary);
  }
  coarseGridFaces{{FACE_LABEL_NAME}}(marker.getSelectedFaceNumber())
    .setNewAboveHanging(marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0, true);
"""

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
"""

    _TEMPLATE_TOUCH_FACE_LAST_TIME = """
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging(fineGridFace{{FACE_LABEL_NAME}}.getNewAboveHanging());
"""

    def operation_body(self, operation_name):
        result = "\n"

        d = {}
        d["FACE_LABEL_NAME"] = UpdateFaceLabel.get_attribute_name(self._solver_name)
        d["SOLVER_NAME"] = self._solver_name

        if operation_name == ActionSet.OPERATION_CREATE_FACE:
            result = jinja2.Template(self._TEMPLATE_CREATE_FACE).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            result = jinja2.Template(self._TEMPLATE_TOUCH_FACE_FIRST_TIME).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
            result = jinja2.Template(self._TEMPLATE_TOUCH_FACE_LAST_TIME).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME:
            result = jinja2.Template(
                self._TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME
            ).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME:
            result = jinja2.Template(
                self._TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME
            ).render(**d)

        return result

    def attributes(self):
        return ""

    def includes(self):
        return ["Abstract{}.h".format(self._solver_name)]


def create_face_label(solver_name):
    result = exahype2.datamodel.MetaData(
        UpdateFaceLabel.get_attribute_name(solver_name)
    )

    result.add_attribute(exahype2.datamodel.Boolean("boundary"))
    result.add_attribute(exahype2.datamodel.Boolean("isHanging"))
    result.add_attribute(exahype2.datamodel.BooleanArray("aboveHanging", "2"))
    result.add_attribute(exahype2.datamodel.BooleanArray("newAboveHanging", "2"))
    result.add_attribute(exahype2.datamodel.BooleanArray("updated", "2"))
    # Mesh transfer rebuilds face halves whose adjacent cells have not projected their data.
    result.add_attribute(exahype2.datamodel.BooleanArray("requiresDataTransfer", "2"))
    result.add_attribute(exahype2.datamodel.DoubleArray("updatedTimeStamp", "2"))
    result.add_attribute(exahype2.datamodel.DoubleArray("newTimeStamp", "2"))
    result.add_attribute(exahype2.datamodel.DoubleArray("oldTimeStamp", "2"))

    result.merge_method_definition = """
  switch (context) {
  case ::peano4::grid::SendReceiveContext::InterRank:
  case ::peano4::grid::SendReceiveContext::IntraRank: {
    setBoundary(getBoundary() or neighbour.getBoundary());

    const int normal         = marker.getSelectedFaceNumber() % DIMENSIONS;
    const int neighbourEntry = marker.outerNormal()(normal) < 0.0 ? 0 : 1;

    setUpdated(neighbourEntry, neighbour.getUpdated(neighbourEntry));
    setRequiresDataTransfer(neighbourEntry, neighbour.getRequiresDataTransfer(neighbourEntry));
    setUpdatedTimeStamp(neighbourEntry, neighbour.getUpdatedTimeStamp(neighbourEntry));
    setNewTimeStamp(neighbourEntry, neighbour.getNewTimeStamp(neighbourEntry));
    setOldTimeStamp(neighbourEntry, neighbour.getOldTimeStamp(neighbourEntry));
    setAboveHanging(neighbourEntry, neighbour.getAboveHanging(neighbourEntry));
    break;
  }
  default:
    assertion(false, "Not implemented");
  }
"""

    return result
