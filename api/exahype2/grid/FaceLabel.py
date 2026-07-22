# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2
import peano4.datamodel


class UpdateFaceLabel(ActionSet):
    """
    An action set that the project merges into all ExaHyPE2 steps automatically.

    User solvers do not have to contribute anything here.

    There are seven fields which have a value per side of the face:

    - Updated: Boolean[2]
      Shows if the corresponding adjacent cell (left vs right) has performed a
      time step.
    - UpdatedTimeStamp: Double[2]
      Holds a new time stamp temporarily until it is rolled over into the
      current time stamp. Only the Finite Volumes solver uses this field.
    - NewTimeStamp: Double[2]
      The current time stamp. It determines, for example, whether a local time
      step is due.
    - OldTimeStamp: Double[2]
      Time stamp of the preceding time step, which the interpolation in time
      requires.
    - AboveHanging: Boolean[2]
      States whether the face sits above a hanging face, i.e. whether the face
      is adjacent to a cell on a finer refinement level. The face directly
      "across" from this one then holds no useful information, so that
      information has to come from the finer level underneath.
    - NewAboveHanging: Boolean[2]
      Collects the AboveHanging state while the traversal ascends.
    - RequiresDataTransfer: Boolean[2]
      Marks a side whose face data has to be rebuilt after a topology change.

    In addition there are two fields that apply to the whole face:

    - Boundary: Boolean
      Shows whether the face is at a domain boundary. Face creation sets it by
      comparing the face position against the domain boundaries.
    - IsHanging: Boolean
      States whether the face is a hanging face, i.e. whether the face is
      adjacent to a cell on a coarser refinement level, so there is no face
      directly "across" from this one for it to merge with.

    ## Initialisation

    The initialisation sets the boundary flag if the face is on the boundary
    and sets all time stamps to zero. The DG solvers require properly set face
    time stamps, while the Finite Volumes solver spots inconsistencies and
    missing data updates more easily when the stamps start from zero.

    ## IsHanging and AboveHanging

    A hanging face does not retain face data between mesh traversals. Its label
    is re-derived on every first touch: the time stamps, IsHanging, Boundary,
    Updated and both AboveHanging fields all start from well-defined values.
    RequiresDataTransfer is the one persistent per-side field the hanging path
    leaves untouched.

    AboveHanging on a persistent face states per side whether finer cells hang
    below the face. It is rebuilt every traversal: touchFaceFirstTime() clears
    NewAboveHanging, each hanging subface sets the entry of its side, and
    touchFaceLastTime() rolls NewAboveHanging over into AboveHanging. Across
    tree boundaries the merge copies the neighbour's entry. ADER-DG uses the
    flag to solve Riemann problems on the fine subfaces and to pick the half of
    the flux buffer that holds the restricted fine fluxes. NewAboveHanging is
    valid in touchFaceLastTime() only, while the traversal ascends.

    See also peano4::datamanagement::FaceMarker::hasBeenRefined().
    """

    def get_attribute_name(solver_name):
        return solver_name + "FaceLabel"

    def __init__(self, solver_name):
        super(UpdateFaceLabel, self).__init__(descend_invocation_order=1)
        self._solver_name = solver_name
        pass

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
  // A hanging face never passes through createFace, so its
  // boundary flag is otherwise never set. If such a face coincides with the
  // domain wall, getBoundary() stays false and HandleBoundary (which is guarded
  // on getBoundary()) skips it - leaving its outer/boundary-side halo at its
  // zero-initialised value. For a system with a non-zero flux at rest (e.g. the
  // Euler pressure) that zero ghost breaks free-stream preservation (p=0 on the
  // boundary face vs p=1 opposite -> the pressure flux does not cancel -> a
  // spurious momentum source in the adjacent cell). We therefore re-evaluate the
  // same geometric boundary test that createFace uses (marker.getX() is the face
  // centre for a hanging FaceMarker) and set the flag here too.
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
    """
    Build the per-face metadata that we maintain for one solver.

    solver_name: string
      Name of the solver
    """
    result = peano4.datamodel.MetaData(UpdateFaceLabel.get_attribute_name(solver_name))

    result.add_attribute(peano4.datamodel.Boolean("boundary"))
    result.add_attribute(peano4.datamodel.Boolean("isHanging"))
    result.add_attribute(peano4.datamodel.BooleanArray("aboveHanging", "2"))
    result.add_attribute(peano4.datamodel.BooleanArray("newAboveHanging", "2"))
    result.add_attribute(peano4.datamodel.BooleanArray("updated", "2"))
    # Per side: the face has just been created and the adjacent cell has not yet
    # re-derived this side's face data. Persistent, so the mesh-transfer passes
    # can restrict their work to faces a topology change actually touched.
    result.add_attribute(peano4.datamodel.BooleanArray("requiresDataTransfer", "2"))
    result.add_attribute(peano4.datamodel.DoubleArray("updatedTimeStamp", "2"))
    result.add_attribute(peano4.datamodel.DoubleArray("newTimeStamp", "2"))
    result.add_attribute(peano4.datamodel.DoubleArray("oldTimeStamp", "2"))

    result.merge_method_definition = """
  switch (context) {
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
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
