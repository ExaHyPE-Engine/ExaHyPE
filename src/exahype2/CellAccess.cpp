#include "CellAccess.h"


exahype2::CellAccess::CellAccess(
  const double* __restrict__ QIn,
  int haloSize,
  int unknowns,
  int numberOfAuxiliaryVariables,
  int numberOfDoFsPerAxisInPatch
):
  _QIn(QIn),
  _haloSize(haloSize),
  _unknowns(unknowns),
  _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables),
  _numberOfDoFsPerAxisInPatch(numberOfDoFsPerAxisInPatch) {}


int exahype2::CellAccess::size() const {
  if constexpr (Dimensions == 2) {
    return (_numberOfDoFsPerAxisInPatch + 2 * _haloSize) * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize)
           * (_unknowns + _numberOfAuxiliaryVariables);
  } else {
    return (_numberOfDoFsPerAxisInPatch + 2 * _haloSize) * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize)
           * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize) * (_unknowns + _numberOfAuxiliaryVariables);
  }
}


const double* __restrict__ exahype2::CellAccess::operator()(
  const tarch::la::Vector<Dimensions, int>& relativeCellPosition
) const {
  if constexpr (Dimensions == 2) {
    int voxelShift = relativeCellPosition(0)
                     + relativeCellPosition(1) * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize);
    return _QIn + voxelShift * (_unknowns + _numberOfAuxiliaryVariables);
  } else {
    int voxelShift = relativeCellPosition(0) + relativeCellPosition(1) * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize)
                     + relativeCellPosition(2) * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize)
                         * (_numberOfDoFsPerAxisInPatch + 2 * _haloSize);
    return _QIn + voxelShift * (_unknowns + _numberOfAuxiliaryVariables);
  }
}


double exahype2::CellAccess::operator()(const tarch::la::Vector<Dimensions, int>& relativeCellPosition, int unknown)
  const {
  return *((*this)(relativeCellPosition) + unknown);
}


const double* __restrict__ exahype2::CellAccess::left(int normal) const {
  tarch::la::Vector<Dimensions, int> relativePosition(0);
  relativePosition(normal) = -1;
  return (*this)(relativePosition);
}


const double* __restrict__ exahype2::CellAccess::right(int normal) const {
  tarch::la::Vector<Dimensions, int> relativePosition(0);
  relativePosition(normal) = 1;
  return (*this)(relativePosition);
}


double exahype2::CellAccess::left(int normal, int unknown) const {
  tarch::la::Vector<Dimensions, int> relativePosition(0);
  relativePosition(normal) = -1;
  return (*this)(relativePosition, unknown);
}


double exahype2::CellAccess::right(int normal, int unknown) const {
  tarch::la::Vector<Dimensions, int> relativePosition(0);
  relativePosition(normal) = 1;
  return (*this)(relativePosition, unknown);
}


double exahype2::CellAccess::centre(int unknown) const { return *(_QIn + unknown); }
