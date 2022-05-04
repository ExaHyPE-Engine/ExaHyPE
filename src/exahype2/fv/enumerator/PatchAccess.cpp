#include "PatchAccess.h"


exahype2::fv::enumerator::PatchAccess::PatchAccess(const double* __restrict__ QIn, int numberOfVolumesPerAxisInPatch, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
  _QIn(QIn),
  _numberOfVolumesPerAxisInPatch(numberOfVolumesPerAxisInPatch),
  _haloSize(haloSize),
  _unknowns(unknowns),
  _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {
}


int exahype2::fv::enumerator::PatchAccess::size() const {
  if (Dimensions==2) {
    return (_numberOfVolumesPerAxisInPatch+2*_haloSize)
        * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
        * (_unknowns+_numberOfAuxiliaryVariables);
  }
  else {
    return (_numberOfVolumesPerAxisInPatch+2*_haloSize)
        * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
        * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
        * (_unknowns+_numberOfAuxiliaryVariables);
  }
}


const double* __restrict__ exahype2::fv::enumerator::PatchAccess::operator()(
  const tarch::la::Vector<Dimensions,int>&  relativeVolumePosition
) const {
  if (Dimensions==2) {
    int voxelShift = relativeVolumePosition(0)
                   + relativeVolumePosition(1) * (_numberOfVolumesPerAxisInPatch+2*_haloSize);
    return _QIn + voxelShift * (_unknowns+_numberOfAuxiliaryVariables);
  }
  else {
    int voxelShift = relativeVolumePosition(0)
                   + relativeVolumePosition(1) * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
                   + relativeVolumePosition(2) * (_numberOfVolumesPerAxisInPatch+2*_haloSize) * (_numberOfVolumesPerAxisInPatch+2*_haloSize);
    return _QIn + voxelShift * (_unknowns+_numberOfAuxiliaryVariables);
  }
}


double exahype2::fv::enumerator::PatchAccess::operator()(
  const tarch::la::Vector<Dimensions,int>&  relativeVolumePosition,
  int unknown
) const {
  return *((*this)(relativeVolumePosition) + unknown);
}


const double* __restrict__ exahype2::fv::enumerator::PatchAccess::left(int normal) const {
  tarch::la::Vector<Dimensions,int> relativePosition(0);
  relativePosition(normal) = -1;
  return (*this)(relativePosition);
}


const double* __restrict__ exahype2::fv::enumerator::PatchAccess::right(int normal) const {
  tarch::la::Vector<Dimensions,int> relativePosition(0);
  relativePosition(normal) = 1;
  return (*this)(relativePosition);
}


double exahype2::fv::enumerator::PatchAccess::left(int normal, int unknown) const {
  tarch::la::Vector<Dimensions,int> relativePosition(0);
  relativePosition(normal) = -1;
  return (*this)(relativePosition,unknown);
}


double exahype2::fv::enumerator::PatchAccess::right(int normal, int unknown) const {
  tarch::la::Vector<Dimensions,int> relativePosition(0);
  relativePosition(normal) = 1;
  return (*this)(relativePosition,unknown);
}


double exahype2::fv::enumerator::PatchAccess::centre(int unknown) const {
  return *(_QIn + unknown);
}
