#include "PeanoCurve.h"
#include "AutomatonState.h"


constexpr int peano4::grid::PeanoCurve::CallStack;
constexpr int peano4::grid::PeanoCurve::NumberOfBaseStacks;


bool peano4::grid::PeanoCurve::isTraversePositiveAlongAxis(
  const AutomatonState&  state,
  int                    axis
) {
  assertion( axis >= 0 );
  assertion( axis < Dimensions );

  std::bitset<Dimensions> influencingEvenFlags = state.getEvenFlags();
  influencingEvenFlags.set(axis,false);

  bool result = ( influencingEvenFlags.count() % 2) == 0;
  return result ^ state.getInverted();
}


peano4::utils::LoopDirection peano4::grid::PeanoCurve::getLoopDirection(
  const AutomatonState&  state
) {
  peano4::utils::LoopDirection result;
  for (int i=0; i<Dimensions; i++) {
    result[i] = isTraversePositiveAlongAxis(state,i);
  }
  return result;
}


void peano4::grid::PeanoCurve::setExitFace(AutomatonState& cell, int axis) {
  if ( isTraversePositiveAlongAxis(cell, axis) ) {
    removeFaceAccessNumber( cell, axis+Dimensions );
    setFaceAccessNumber( cell, axis+Dimensions, 1 );
  }
  else {
    removeFaceAccessNumber( cell, axis );
    setFaceAccessNumber( cell, axis, 1 );
  }
}


void peano4::grid::PeanoCurve::setEntryFace(AutomatonState& cell, int axis) {
  if ( isTraversePositiveAlongAxis(cell, axis) ) {
    removeFaceAccessNumber( cell, axis );
    setFaceAccessNumber( cell, axis, -1 );
  }
  else {
    removeFaceAccessNumber( cell, axis+Dimensions );
    setFaceAccessNumber( cell, axis+Dimensions, -1 );
  }
}


void peano4::grid::PeanoCurve::invertEvenFlag(AutomatonState& cell, int axis) {
  assertion( axis >= 0 );
  assertion( axis < Dimensions );
  cell.flipEvenFlags( axis );
}


void peano4::grid::PeanoCurve::removeFaceAccessNumber(AutomatonState& cell, int face) {
  assertion( face >= 0 );
  assertion( face < 2*Dimensions );

  // use auto here as I tend to change the type of the access flags
  auto accessNumber = cell.getAccessNumber();
  int oldFaceNumber = accessNumber(face);

  if (accessNumber(face)>0) {
    for (int i=0; i<2*Dimensions; i++)
      if ( accessNumber(i) >= oldFaceNumber ) accessNumber(i)--;
  }
  if (accessNumber(face)<0) {
    for (int i=0; i<2*Dimensions; i++)
      if ( accessNumber(i) <= oldFaceNumber ) accessNumber(i)++;
  }

  accessNumber(face) = 0;
  cell.setAccessNumber(accessNumber);
}


std::bitset<Dimensions> peano4::grid::PeanoCurve::getFirstVertexIndex( const AutomatonState& cell ) {
  std::bitset<Dimensions> result;
  std::bitset<Dimensions> currentMask;

  for (int i=0; i<Dimensions; i++) {
    if ( cell.getEvenFlags(i) ) {
      currentMask[i] = true;
      currentMask.flip();

      result ^= currentMask;

      currentMask.flip();
      currentMask[i] = false;
    }
  }

  if (cell.getInverted()) {
	result.flip();
  }

  return result;
}


void peano4::grid::PeanoCurve::setFaceAccessNumber(AutomatonState& cell, int face, int value) {
  assertion( face >= 0 );
  assertion( face < 2*Dimensions );
  assertion( value != 0 );
  assertion( cell.getAccessNumber()(face) == 0 );

  // use auto here as I tend to change the type of the access flags
  auto accessNumber = cell.getAccessNumber();

  if (value > 0 ) {
    for (int i=0; i<2*Dimensions; i++)
      if ( accessNumber(i) >= value ) accessNumber(i)++;
  }
  else {
    for (int i=0; i<2*Dimensions; i++)
      if ( accessNumber(i) <= value ) accessNumber(i)--;
  }

  accessNumber(face) = static_cast<short int>(value);
  cell.setAccessNumber(accessNumber);
}


int peano4::grid::PeanoCurve::getInputStackNumber(const AutomatonState& state) {
  return NumberOfBaseStacks + (state.getInverted() ? -1 : -2);
}


int peano4::grid::PeanoCurve::getOutputStackNumber(const AutomatonState& state) {
  return NumberOfBaseStacks + (state.getInverted() ? -2 : -1);
}


int peano4::grid::PeanoCurve::getVertexReadStackNumber(const AutomatonState&  state, const std::bitset<Dimensions>&  vertex ) {
  int smallestValue = -2*Dimensions-1;
  int result        = getInputStackNumber(state);
  int direction     = -1;

  for (int d=0; d<Dimensions; d++) {
    const int face = vertex[d]==0 ? d : d+Dimensions;
    const int faceAccessNumber = state.getAccessNumber(face);
    if (faceAccessNumber<0 && faceAccessNumber>smallestValue) {
      result        = face + NumberOfBaseStacks;
      smallestValue = faceAccessNumber;
      direction     = d;
    }
  }

  if ( direction>=0 and state.getEvenFlags(direction) ) {
    result = (result-NumberOfBaseStacks)<Dimensions ? result+Dimensions : result-Dimensions;
  }

  assertion1( result>CallStack, result);
  return result;
}


bool peano4::grid::PeanoCurve::isInOutStack( int number ) {
  return number>CallStack and number < NumberOfBaseStacks;
}


bool peano4::grid::PeanoCurve::isTemporaryStack( int number ) {
  return number >= NumberOfBaseStacks;
}


int peano4::grid::PeanoCurve::getVertexWriteStackNumber(const AutomatonState& cell, const std::bitset<Dimensions>& vertex ) {
  int biggestValue = 2*Dimensions+1;
  int result       = getOutputStackNumber(cell);
  int direction    = -1;

  for (int d=0; d<Dimensions; d++) {
    const int face = vertex[d]==0 ? d : d+Dimensions;
    const int faceAccessNumber = cell.getAccessNumber(face);
    if (faceAccessNumber>0 && faceAccessNumber<biggestValue) {
      result       = face + NumberOfBaseStacks;
      biggestValue = faceAccessNumber;
      direction    = d;
    }
  }

  if ( direction>=0 and cell.getEvenFlags(direction) ) {
    result = (result-NumberOfBaseStacks)<Dimensions ? result+Dimensions : result-Dimensions;
  }

  assertion1( result>CallStack, result);
  return result;
}


int peano4::grid::PeanoCurve::getFaceNumberAlongCurve(const AutomatonState& cell, int face ) {
  if (cell.getInverted()) {
    face = 2*Dimensions - 1 - face;
  }
  int normal = face % Dimensions;
  int result = face;
  for (int d=0; d<Dimensions; d++) {
    if (d!=normal) {
      if (cell.getEvenFlags(d) and not cell.getInverted()) {
        result += Dimensions;
      }
      else if (cell.getEvenFlags(d) and cell.getInverted()) {
        result += Dimensions;
      }
    }
  }

  result = result % (2*Dimensions);
  return result;
}


int peano4::grid::PeanoCurve::getFaceReadStackNumber(const AutomatonState& state, int  face ) {
  int result       = getInputStackNumber(state);
  int direction    = face % Dimensions;

  if (state.getAccessNumber(face)<0) {
    int newResult = face;
    if (state.getInverted())           newResult += Dimensions;
    if (state.getEvenFlags(direction)) newResult += Dimensions;
    newResult = newResult % (2*Dimensions);
    result = newResult + NumberOfBaseStacks;
  }

  assertion1( result>CallStack, result);
  return result;
}


int peano4::grid::PeanoCurve::getFaceWriteStackNumber(const AutomatonState& cell, int  face ) {
  int result       = getOutputStackNumber(cell);
  int direction    = face % Dimensions;

  if (cell.getAccessNumber(face)>0) {
    int newResult = face;
    if (cell.getInverted())           newResult += Dimensions;
    if (cell.getEvenFlags(direction)) newResult += Dimensions;
    newResult = newResult % (2*Dimensions);
    result = newResult + NumberOfBaseStacks;
  }

  assertion1( result>CallStack, result);
  return result;
}


int peano4::grid::PeanoCurve::getCellReadStackNumber(const AutomatonState& state) {
  return getInputStackNumber(state);
}


int peano4::grid::PeanoCurve::getCellWriteStackNumber(const AutomatonState& state) {
  return getOutputStackNumber(state);
}
