// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <functional>
#include <string>

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"


namespace exahype2 {
  /**
   * Cell access class
   *
   * This class is to be used by user code which do not only want to work
   * cell-wisely, but also need access to neighbour cells.
   * For example, it can be used in the source term code to get access to
   * the neighbour cell to evaluate a second derivative.
   *
   * @param haloSize For a Finite Volume solver, this is 1 if each
   *   patch is augmented with one halo volume around it. For a DG solver,
   *   it is 1 if you project only one quantity, i.e., the solution. However,
   *   you might have a value bigger than 1 if you also project higher order
   *   properties such as the first derivative along the normal. Some
   *   code snippets within ExaHyPE call the haloSize overlap.
   */
  class CellAccess {
  public:
    CellAccess(
      const double* __restrict__ QIn,
      int haloSize,
      int unknowns,
      int numberOfAuxiliaryVariables,
      int numberOfDoFsPerAxisInPatch
    );

    ~CellAccess() = default;

    int size() const;

    /**
     * Get a pointer to another cell's data. Pass in {0,0,0} and 0 as
     * unknown, and you will get the current cell for which you have
     * created the CellAccess object. Pass in {0,0,0} and k to access
     * its kth unknown. Pass in {-1,0,0} for example to access the left
     * neighbour of a cell.
     */
    const double* __restrict__ operator()(const tarch::la::Vector<Dimensions, int>& relativeCellPosition) const;
    double operator()(const tarch::la::Vector<Dimensions, int>& relativeCellPosition, int unknown) const;

    double centre(int unknown) const;

    /**
     * Return access to the left neighbour. Left means left along coordinate
     * axis normal.
     */
    const double* __restrict__ left(int normal) const;
    const double* __restrict__ right(int normal) const;

    double left(int normal, int unknown) const;
    double right(int normal, int unknown) const;

    std::string toString() const;

  private:
    const double* __restrict__ _QIn;
    const int _haloSize;
    const int _unknowns;
    const int _numberOfAuxiliaryVariables;
    const int _numberOfDoFsPerAxisInPatch;
  };
} // namespace exahype2
