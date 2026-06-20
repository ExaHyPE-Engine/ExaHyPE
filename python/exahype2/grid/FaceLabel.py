# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2
import peano4.datamodel


class UpdateFaceLabel(ActionSet):
    def get_attribute_name(solver_name):
        return solver_name + "FaceLabel"

    """
   SetLabels is an action set which is automatically merged into all ExaHyPE2
   steps by the project. There's nothing for the user solvers to be done here.

   There are four fields which have a value per side of the face:

   - Updated: Boolean[2]
     Shows if the corresponding adjacent cell (left vs right) has performed a
     time step.
   - UpdatedTimeStamp: Double[2]
     Used only by the Finite Volumes solver so far to store a new time stamp
     temporarily which is later on rolled over into the real time stamp.
   - NewTimeStamp: Double[2]
     New (and also current time stamp). This time stamp determines for example
     if you want to do a local time step or not.
   - OldTimeStamp: Double[2]
     Time stamp of previous time step. The old time stamp is required for the
     in-time interpolation, e.g.
   - AboveHanging: Boolean[2]
     This states whether the face is above a hanging face, i.e. whether the
     face is adjacent to a cell with a finer refinement level and therefore
     the face directly "across" from this one will likely contain no useful
     information, meaning said information must be obtained from the finer
     level underneath this one.

  In addition there are three fields that are in common to both faces:

   - Boundary: Boolean
     Shows whether the cell is at a domain boundary. This is set upon creation
     by checking one's position in relation to the domain boundaries.
   - IsHanging: Boolean
     This states whether the face is a hanging face, i.e. whether the face
     is adjacent to a cell with a coarser refinement level and therefore there
     is no face directly "across" from this one for it to merge with.

   ## Initialisation

   The initialisation sets the boundary flag if the face is on the boundary.
   It also sets the time stamps all to zero. This is actually a tricky thing
   to do, so I'm not sure if it is a good idea: the DG solvers for example
   require the face time stamps to be set properly. The Finite Volumes solver
   in return benefits if it is not (properly) initialised, as it then is
   simpler to spot inconsistencies and missing data updates.

   ## IsHanging and AboveHanging
   TODO: Outdated documentation
   IsHanging is a redundant information. If a face is hanging, it is not held
   persistently. Consequently, we get a createHangingFace() in each and every
   mesh traversal and hence know implicitly that the face is hanging. The flag
   therefore replicates information which is implicitly available anyway.

   AboveHanging is stored persistently. At the moment, it is only used by
   ADER-DG solvers. Most solvers should be able to work without this additional
   information. For ADER-DG, it is used to ensure that Riemann-solves
   are performed on fine faces and not on coarse faces, as the later would
   cause unphysical transfer of information which lead to uncontrolled
   oscillations.

   AboveHanging is historical information. If the mesh is just about to refine,
   then the flag might not be set even though we literally just add new
   "subfaces". If a mesh coarsens, the flag might be set, even though we
   literally just remove the adjacent fine grid cells. If you need to know what
   happened in this very iteration, you can read out the flag NewAboveHanging,
   but this one is only valid in touchFaceLastTime(), i.e. while we ascend in
   the tree.

   Please consult peano4::datamanagement::FaceMarker::hasBeenRefined() in this
   context, too.
  """

    def __init__(self, solver_name):
        super(UpdateFaceLabel, self).__init__(
            descend_invocation_order=1, parallel=False
        )
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
  tarch::la::Vector<DIMENSIONS, double> offset(DomainOffset);
  tarch::la::Vector<DIMENSIONS, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<DIMENSIONS; d++) {
    isBoundary |= tarch::la::equals( marker.getX()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.getX()(d), offset(d) + size(d) );
  }
  fineGridFace{{FACE_LABEL_NAME}}.setBoundary( isBoundary );
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setNewAboveHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp( 0.0 );
"""

    _Template_TouchHangingFaceFirstTime = """
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging(true);
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

    Build up the metadata structure that we need per face to maintain
    per-face data per solver.

    solver_name: string
      Name of the solver

    """
    result = peano4.datamodel.MetaData(UpdateFaceLabel.get_attribute_name(solver_name))

    result.add_attribute(peano4.datamodel.Boolean("boundary"))
    result.add_attribute(peano4.datamodel.Boolean("isHanging"))
    result.add_attribute(peano4.datamodel.BooleanArray("aboveHanging", "2"))
    result.add_attribute(peano4.datamodel.BooleanArray("newAboveHanging", "2"))
    result.add_attribute(peano4.datamodel.BooleanArray("updated", "2"))
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
        setUpdatedTimeStamp( neighbourEntry, neighbour.getUpdatedTimeStamp( neighbourEntry ) );
        setNewTimeStamp( neighbourEntry, neighbour.getNewTimeStamp( neighbourEntry ) );
        setOldTimeStamp( neighbourEntry, neighbour.getOldTimeStamp( neighbourEntry ) );
        setAboveHanging( neighbourEntry, neighbour.getAboveHanging( neighbourEntry ) );
      }
      break;
    default:
      assertion(false, "not implemented yet");
  }
"""

    return result
