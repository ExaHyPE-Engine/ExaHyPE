#include "{CLASSNAME}.h"


{FULL_QUALIFIED_CLASSNAME}::{CLASSNAME}():
  _timeStamp(0.0),
  _projectOntoFace(true),
  _solveCell(false) {{
}}


double {FULL_QUALIFIED_CLASSNAME}::getMinTimeStamp() const {{
  return _timeStamp;
}}


double {FULL_QUALIFIED_CLASSNAME}::getMaxTimeStamp() const {{
  return _timeStamp;
}}


double {FULL_QUALIFIED_CLASSNAME}::getMinTimeStepSize() const {{
  return {TIME_STEP_SIZE};
}}


double {FULL_QUALIFIED_CLASSNAME}::getMaxTimeStepSize() const {{
  return {TIME_STEP_SIZE};
}}


void {FULL_QUALIFIED_CLASSNAME}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {{
}}



void {FULL_QUALIFIED_CLASSNAME}::finishTimeStep() {{
  if ( _projectOntoFace ) {{
    _projectOntoFace = false;
    _solveCell       = true;
  }}
  else if (_solveCell) {{
    _solveCell       = false;
    _projectOntoFace = true;
    _timeStamp      += {TIME_STEP_SIZE};
  }}
}}


bool {FULL_QUALIFIED_CLASSNAME}::startNewTimeStepOnCellAndProjectOntoFace() const {{
  return _projectOntoFace;
}}


bool {FULL_QUALIFIED_CLASSNAME}::solveCellProblemAndFinishTimeStepOnCell() const {{
  return _solveCell;
}}


::exahype2::RefinementCommand {FULL_QUALIFIED_CLASSNAME}::refinementCriterion(
  double* Q,
  const tarch::la::Vector<Dimensions,double>&  cellCentre,
  const tarch::la::Vector<Dimensions,double>&  cellH,
  double                                       t
) {{
  return ::exahype2::RefinementCommand::Keep;
}}


void {FULL_QUALIFIED_CLASSNAME}::adjustSolution(
  double * __restrict__ Q,
  double* Q,
  const tarch::la::Vector<Dimensions,double>&  cellCentre,
  const tarch::la::Vector<Dimensions,double>&  cellH,
  double                                       t
) {{

}}


void {FULL_QUALIFIED_CLASSNAME}::boundaryConditions(
  double* Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  faceH,
  double                                       t,
  const tarch::la::Vector<Dimensions,double>&  normal
) {{

}}



void {FULL_QUALIFIED_CLASSNAME}::solveCell(
  double* QCell,
  double* QLeftFace,
  double* QBottomFace,
  double* QRightFace,
  double* QUpperFace,
  const tarch::la::Vector<Dimensions,double>&  cellCentre,
  const tarch::la::Vector<Dimensions,double>&  cellH,
  double                                       t
) {{

}}


void {FULL_QUALIFIED_CLASSNAME}::projectOntoFaces(
  double* QCell,
  double* QLeftFace,
  double* QBottomFace,
  double* QRightFace,
  double* QUpperFace,
  const tarch::la::Vector<Dimensions,double>&  cellCentre,
  const tarch::la::Vector<Dimensions,double>&  cellH,
  double                                       t
) {{
  // Handle left face
  // ----------------
  // Face 0 is the left face (see guidebook). Entries within face are
  // enumerated in z-order and we assume the face holds a nx2 matrix, i.e.
  // index 0 belongs to the left neighbour, index 1 to the right one,
  // index 2 to the left one, ...
  //
  // Consequently, we only overwrite entries 1,3,5,... when we project
  // the upper left triangle to the face. I assume in this pseudo
  // implementation that triangles are arranged like
  //
  // |-----/
  // |    /|
  // |   / |
  // |  /  |
  // | /   |
  // |/----|
  //
  // I furthermore assume that the degrees of freedoms are scalar and that
  // I first enumerate the 3 dofs of the lower triangle (again z-wisely) and
  // then the guys in the upper triangle z-wisely.
  //
  faceData[0][1] = cellData[3];  // let the 3 be the left bottom unknown in upper triangle
  faceData[0][3] = cellData[4];  // let the 4 the left top unknown in upper triangle
  faceData[0][5] = cellData[5];  // let the 4 the left top unknown in upper triangle

  // Handle bottom face
  // ------------------
  // Bottom face is face number 1
  faceData[1][4] = cellData[0];  // let the 3 be the left bottom unknown in upper triangle
  faceData[1][5] = cellData[1];  // let the 4 the left top unknown in upper triangle
  faceData[1][6] = cellData[2];  // let the 4 the left top unknown in upper triangle

  // ...
}}
