#include "AoSLexicographicVolumeEnumerator.h"


std::string exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator::toString() const {
  std::ostringstream msg;
  msg << "(AoS-lex"
      << ",#patches=" << _numberOfPatches
      << ",#vols=" << _numberOfVolumesPerAxisInPatch
      << ",#halo=" << _haloSize
      << ",#unknowns=" << _unknowns
      << ",#aux=" << _numberOfAuxiliaryVariables
      << ")";

  #if PeanoDebug>0
  msg << ", index-sequence=["
      << "," << (*this)(0,{0,0},0)
      << "," << (*this)(1,{0,0},0)
      << ",..."
      << "," << (*this)(0,{1,0},0)
      << "," << (*this)(0,{2,0},0)
      << ",..."
      << "," << (*this)(0,{0,1},0)
      << "," << (*this)(0,{0,2},0)
      << ",..."
      << "," << (*this)(0,{0,0},1)
      << "," << (*this)(0,{0,0},2)
      << ",..."
      << "," << this->size()
      << "]"
      ;
  #endif

  return msg.str();
}


int exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator::size() const {
  #if Dimensions==2
  return _numberOfPatches
      * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
      * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
      * (_unknowns+_numberOfAuxiliaryVariables);
  #else
  return _numberOfPatches
      * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
      * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
      * (_numberOfVolumesPerAxisInPatch+2*_haloSize)
      * (_unknowns+_numberOfAuxiliaryVariables);
  #endif
}
