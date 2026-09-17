# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2
import peano4.datamodel


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

    def get_constructor_body(self):
        return ""

    def get_destructor_body(self):
        return ""

    _Template_TouchFaceFirstTime = """
  fineGridFace{{FACE_LABEL_NAME}}.setUpdated( false );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging( false );
"""

    _Template_CreateFace = """
  const tarch::la::Vector<DIMENSIONS, double> offset(DomainOffset);
  const tarch::la::Vector<DIMENSIONS, double> size(DomainSize);
  bool isBoundary = false;
  for (int d = 0; d < DIMENSIONS; d++) {
    isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d));
    isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d) + size(d));
  }
  fineGridFace{{FACE_LABEL_NAME}}.setBoundary( isBoundary );
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setRequiresDataTransfer( 0, true );
  fineGridFace{{FACE_LABEL_NAME}}.setRequiresDataTransfer( 1, true );
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp( 0.0 );
"""

    _Template_TouchHangingFaceFirstTime = """
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging(true);
  // A face that has just turned hanging starts from whatever label its storage
  // slot held before, and the hanging-face path never re-derives these fields.
  // The ADER-DG corrector selects its flux half through AboveHanging, so a
  // leftover value makes a cell consume its neighbour's half of the flux.
  fineGridFace{{FACE_LABEL_NAME}}.setUpdated( false );
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging( false );
  // A hanging face never passes through createFace, so the boundary flag is set here with
  // the geometric test createFace uses. marker.getX() is the centre of the hanging face.
  {
    const tarch::la::Vector<DIMENSIONS, double> offset(DomainOffset);
    const tarch::la::Vector<DIMENSIONS, double> size(DomainSize);
    bool isBoundary = false;
    for (int d = 0; d < DIMENSIONS; d++) {
      isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d));
      isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d) + size(d));
    }
    fineGridFace{{FACE_LABEL_NAME}}.setBoundary(isBoundary);
  }
  coarseGridFaces{{FACE_LABEL_NAME}}(marker.getSelectedFaceNumber()).setNewAboveHanging( marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0, true );
"""

    _Template_TouchHangingFaceLastTime = """
"""

    _Template_TouchFaceLastTime = """
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging( fineGridFace{{FACE_LABEL_NAME}}.getNewAboveHanging() );
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_body_of_operation(self, operation_name):
        result = "\n"

        d = {}
        d["FACE_LABEL_NAME"] = UpdateFaceLabel.get_attribute_name(self._solver_name)

        if operation_name == ActionSet.OPERATION_CREATE_FACE:
            result = jinja2.Template(self._Template_CreateFace).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
            result = jinja2.Template(self._Template_TouchFaceLastTime).render(**d)
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME:
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **d
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME:
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **d
            )

        return result

    def get_attributes(self):
        return ""

    def get_includes(self):
        return """
#include "Constants.h"
"""


def create_face_label(solver_name):
    result = peano4.datamodel.MetaData(UpdateFaceLabel.get_attribute_name(solver_name))

    result.add_attribute(peano4.datamodel.Boolean("boundary"))
    result.add_attribute(peano4.datamodel.Boolean("isHanging"))
    result.add_attribute(peano4.datamodel.BooleanArray("aboveHanging", "2"))
    result.add_attribute(peano4.datamodel.BooleanArray("newAboveHanging", "2"))
    result.add_attribute(peano4.datamodel.BooleanArray("updated", "2"))
    # Marks per side that the adjacent cell has not built this side's face data yet, so the
    # mesh-transfer passes visit only the faces a topology change touched.
    result.add_attribute(peano4.datamodel.BooleanArray("requiresDataTransfer", "2"))
    result.add_attribute(peano4.datamodel.DoubleArray("updatedTimeStamp", "2"))
    result.add_attribute(peano4.datamodel.DoubleArray("newTimeStamp", "2"))
    result.add_attribute(peano4.datamodel.DoubleArray("oldTimeStamp", "2"))

    result.merge_method_definition = """
  switch (context) {
    case ::peano4::grid::SendReceiveContext::InterRank:
    case ::peano4::grid::SendReceiveContext::IntraRank:
      {
        setBoundary( getBoundary() or neighbour.getBoundary() );

        const int normal         = marker.getSelectedFaceNumber() % DIMENSIONS;
        const int neighbourEntry = marker.outerNormal()(normal)<0.0 ? 0 : 1;

        setUpdated( neighbourEntry, neighbour.getUpdated( neighbourEntry ) );
        setRequiresDataTransfer( neighbourEntry, neighbour.getRequiresDataTransfer( neighbourEntry ) );
        setUpdatedTimeStamp( neighbourEntry, neighbour.getUpdatedTimeStamp( neighbourEntry ) );
        setNewTimeStamp( neighbourEntry, neighbour.getNewTimeStamp( neighbourEntry ) );
        setOldTimeStamp( neighbourEntry, neighbour.getOldTimeStamp( neighbourEntry ) );
        setAboveHanging( neighbourEntry, neighbour.getAboveHanging( neighbourEntry ) );
      }
      break;
    default:
      assertion(false, "Not implemented yet");
  }
"""

    return result
