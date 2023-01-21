// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace enumerator {
    /**
     * Array of struct enumerator
     *
     * This is the default enumerator for all DoFs in ExaHyPE 2.
     *
     * We assume that we have a lexicographic ordering of the patch data or
     * DG degree of freedom (quadrature points) and all data therein are
     * ordered lexicographically and stored in as AoS: All data are
     *
     * - organised as array of structs, i.e. if you store quantities (a,b,c) per
     *   degree of freedom (dof), then the array first holds the a value of the first
     *   dof, then the b value, then the c value, and then it continues with the
     *   a value of the next dof.
     * - organised lexicographically, i.e. we first enumerate all the degrees of
     *   freedom along the x-axis, then along the y-axis, then along the z-axis.
     * - organised from left to right, bottom-up and front to back.
     * - organised one patch after another, i.e. we first enumerate all the values
     *   of the first patch, before we continue with any value of the second
     *   patch.
     *
     * @see Enumerator.h for a generic description of attribute semantics.
     *
     *
     * @param numberOfCells_ The cells are enumerated one after another, i.e.
     *   we work with an array of cells hosting arrays of structs.
     *
     * @param haloSize Number of dofs around the cell which do not carry
     *   active information, i.e. data you write to.
     */
    struct AoSLexicographicEnumerator {
      public:
        AoSLexicographicEnumerator(int numberOfCells, int numberOfDoFsPerAxisInCell, int haloSize, int unknowns, int numberOfAuxiliaryVariables):
          _numberOfCells(numberOfCells),
          _numberOfDoFsPerAxisInCell(numberOfDoFsPerAxisInCell),
          _haloSize(haloSize),
          _unknowns(unknowns),
          _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables){}

        /**
         * Access an index
         *
         * The index always refers to the interior of the patches. So you can use negative
         * indices if you want.
         *
         * @todo modulo Operator muss ich noch dokumentieren
         */
        int operator()(
          int cellIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int unknown
        ) const InlineMethod {
          if (Dimensions==2) {
            return (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 +      (volumeIndex(1)+_haloSize) *                                                (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 +      (volumeIndex(0)+_haloSize) *                                                                                               (_unknowns+_numberOfAuxiliaryVariables)
                 +                        unknown;
          }
          else {
            return (cellIndex % _numberOfCells) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 +      (volumeIndex(2)+_haloSize) *                                                (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 +      (volumeIndex(1)+_haloSize) *                                                                                               (_numberOfDoFsPerAxisInCell+2*_haloSize) * (_unknowns+_numberOfAuxiliaryVariables)
                 +      (volumeIndex(0)+_haloSize) *                                                                                                                                              (_unknowns+_numberOfAuxiliaryVariables)
                 +                        unknown;
          }
        }

        #if !defined(GPUOffloadingOMP)
        std::string toString() const;
        #endif

        int size() const {
          if (Dimensions==2) {
            return _numberOfCells
                * (_numberOfDoFsPerAxisInCell+2*_haloSize)
                * (_numberOfDoFsPerAxisInCell+2*_haloSize)
                * (_unknowns+_numberOfAuxiliaryVariables);
          }
          else {
            return _numberOfCells
                * (_numberOfDoFsPerAxisInCell+2*_haloSize)
                * (_numberOfDoFsPerAxisInCell+2*_haloSize)
                * (_numberOfDoFsPerAxisInCell+2*_haloSize)
                * (_unknowns+_numberOfAuxiliaryVariables);
          }
        }

        int numberOfDofsPerCell() const {
          if (Dimensions==2) {
            return (_numberOfDoFsPerAxisInCell+2*_haloSize)
                 * (_numberOfDoFsPerAxisInCell+2*_haloSize);
          }
          else {
            return (_numberOfDoFsPerAxisInCell+2*_haloSize)
                 * (_numberOfDoFsPerAxisInCell+2*_haloSize)
                 * (_numberOfDoFsPerAxisInCell+2*_haloSize);
          }
        }

        const int _numberOfCells;
        const int _numberOfDoFsPerAxisInCell;
        const int _haloSize;
        const int _unknowns;
        const int _numberOfAuxiliaryVariables;
    };
  }
}



