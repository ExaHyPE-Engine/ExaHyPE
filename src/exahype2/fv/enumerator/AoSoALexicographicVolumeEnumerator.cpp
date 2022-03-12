#include "AoSoALexicographicVolumeEnumerator.h"



std::string exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator::toString() const {
  std::ostringstream msg;
  msg << "(AoS-lex"
      << ",#patches=" << _numberOfPatches
      << ",#vols=" << _numberOfVolumesPerAxisInPatch
      << ",#halo=" << _haloSize
      << ",#unknowns=" << _unknowns
      << ",#aux=" << _numberOfAuxiliaryVariables
      << ")";

  msg << "," << (*this)(0,{0,0},0)
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
      << "," << (*this)(_numberOfPatches-1,{_numberOfVolumesPerAxisInPatch-1,_numberOfVolumesPerAxisInPatch-1},_numberOfAuxiliaryVariables+_unknowns-1)
      ;

  return msg.str();
}

