#pragma once

#include "AbstractSWESolver.h"
#include "tarch/logging/Log.h"
#include "peano4/utils/Loop.h"

namespace applications {
  namespace exahype2 {
    namespace avalanche {

      enum derivatives { h_x=1, h_y, u_x, u_y, u_xx, u_yy, u_xy, v_x, v_y, v_xx, v_yy, v_xy };
      enum unknowns { h = 0, hu, hv };

      class SWESolver;

    } // namespace avalanche
  }   // namespace exahype2
} // namespace applications


class applications::exahype2::avalanche::SWESolver: public applications::exahype2::avalanche::AbstractSWESolver {
private:
  static tarch::logging::Log _log;

public:
  /**
   * Refinement criterion
   *
   * ExaHypE2 is guided by a maximum and minimum mesh (patch) size.
   * All (dynamic) AMR is constrained by these values, i.e. if your
   * mesh is coarser than the maximum mesh size, ExaHyPE 2 will
   * automatically refine. If you try to refine further than the
   * minimum mesh size, ExaHyPE 2 will ignore any refinement.
   *
   * Consequently, you are fine if you work with a regular mesh:
   * You set the maximum mesh size, and you leave everything else
   * to Peano 4/ExaHyPE 2. If you want to have an adaptive mesh,
   * use this routine to implement the refinement pattern.
   *
   * @param Q This is the (current) solution. The data is not set
   *  to a valid value throughout grid construction. That is: If
   *  t equals 0.0, you cannot assume that Q is properly initialised.
   *  Therefore, Q usually is only evaluated by dynamic AMR codes
   *  which make the solution follow
   */
  virtual ::exahype2::RefinementCommand refinementCriterion(
    const double* __restrict__ Q, // Q[3+11],
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t
  ) override;


  virtual void initialCondition(
    double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    bool                                         gridIsConstructred
  ) override;


  virtual void boundaryConditions(
    const double* __restrict__ Qinside, // Qinside[3+11]
    double* __restrict__ Qoutside,      // Qoutside[3+11]
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    int                                          normal
  ) override;


public:
  /**
   * Determine max eigenvalue over Jacobian in a given point with solution values
   * (states) Q. All parameters are in.
   *
   * @return Max eigenvalue. Result has to be positive, so we are actually speaking
   *   about the maximum absolute eigenvalue.
   */
  virtual double maxEigenvalue(
    const double* __restrict__ Q, // Q[3+11],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) override;


  virtual void flux(
    const double* __restrict__ Q, // Q[3+11],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F // F[3]
  ) override;


  virtual void nonconservativeProduct(
    const double* __restrict__ Q,      // Q[3+11],
    const double* __restrict__ deltaQ, // deltaQ[3+11]
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ BTimesDeltaQ // BTimesDeltaQ[3]
  ) override;

  virtual void sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    double* __restrict__ S
  ) override;

  static inline void extrapolateHalo(double* __restrict__ oldQWithHalo) {
    // extrapolation
    tarch::la::Vector<Dimensions, int> topLeftCell     = {0, NumberOfFiniteVolumesPerAxisPerPatch + 1};
    tarch::la::Vector<Dimensions, int> topRightCell    = {NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftCell  = {0, 0};
    tarch::la::Vector<Dimensions, int> bottomRightCell = {NumberOfFiniteVolumesPerAxisPerPatch + 1, 0};

    tarch::la::Vector<Dimensions, int> topLeftCellN1 = topLeftCell;
    topLeftCellN1(0)++;
    tarch::la::Vector<Dimensions, int> topLeftCellN2 = topLeftCell;
    topLeftCellN2(1)--;

    tarch::la::Vector<Dimensions, int> topRightCellN1 = topRightCell;
    topRightCellN1(0)--;
    tarch::la::Vector<Dimensions, int> topRightCellN2 = topRightCell;
    topRightCellN2(1)--;

    tarch::la::Vector<Dimensions, int> bottomLeftCellN1 = bottomLeftCell;
    bottomLeftCellN1(0)++;
    tarch::la::Vector<Dimensions, int> bottomLeftCellN2 = bottomLeftCell;
    bottomLeftCellN2(1)++;

    tarch::la::Vector<Dimensions, int> bottomRightCellN1 = bottomRightCell;
    bottomRightCellN1(0)--;
    tarch::la::Vector<Dimensions, int> bottomRightCellN2 = bottomRightCell;
    bottomRightCellN2(1)++;

    const int topLeftCellSerialised  = peano4::utils::dLinearised(topLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int topRightCellSerialised = peano4::utils::dLinearised(topRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);

    const int bottomLeftCellSerialised  = peano4::utils::dLinearised(bottomLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int bottomRightCellSerialised = peano4::utils::dLinearised(bottomRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);

    const int topLeftCellN1Serialised = peano4::utils::dLinearised(topLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int topLeftCellN2Serialised = peano4::utils::dLinearised(topLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2);

    const int topRightCellN1Serialised = peano4::utils::dLinearised(topRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int topRightCellN2Serialised = peano4::utils::dLinearised(topRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2);

    const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(bottomLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(bottomLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2);

    const int bottomRightCellN1Serialised = peano4::utils::dLinearised(bottomRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int bottomRightCellN2Serialised = peano4::utils::dLinearised(bottomRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2);

    for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {

      oldQWithHalo[(topLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (oldQWithHalo[(topLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + oldQWithHalo[(topLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);
      oldQWithHalo[(topRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (oldQWithHalo[(topRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + oldQWithHalo[(topRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      oldQWithHalo[(bottomLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (oldQWithHalo[(bottomLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + oldQWithHalo[(bottomLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      oldQWithHalo[(bottomRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (oldQWithHalo[(bottomRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + oldQWithHalo[(bottomRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);
    }
  }

  static inline void calculateDerivatives(double* __restrict__ oldQWithHalo) {
    // derivative calculation
    // backward difference for single derivatives
    // central difference for double derivatives
    const int totalVariables = NumberOfUnknowns + NumberOfAuxiliaryVariables;

    for (int y = 1; y < NumberOfFiniteVolumesPerAxisPerPatch + 1; y++) {
      for (int x = 1; x < NumberOfFiniteVolumesPerAxisPerPatch + 1; x++) {
        const tarch::la::Vector<Dimensions, int> currentCell               = {x, y};
        const tarch::la::Vector<Dimensions, int> xLeftNeighbourCell        = {x - 1, y};
        const tarch::la::Vector<Dimensions, int> xRightNeighbourCell       = {x + 1, y};
        const tarch::la::Vector<Dimensions, int> yLeftNeighbourCell        = {x, y - 1};
        const tarch::la::Vector<Dimensions, int> yRightNeighbourCell       = {x, y + 1};
        const tarch::la::Vector<Dimensions, int> xLeftYLeftNeighbourCell   = {x - 1, y - 1};
        const tarch::la::Vector<Dimensions, int> xLeftYRightNeighbourCell  = {x - 1, y + 1};
        const tarch::la::Vector<Dimensions, int> xRightYLeftNeighbourCell  = {x + 1, y - 1};
        const tarch::la::Vector<Dimensions, int> xRightYRightNeighbourCell = {x + 1, y + 1};


        const int currentCellSerialized         = peano4::utils::dLinearised(currentCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
        const int xLeftNeighbourCellSerialized  = peano4::utils::dLinearised(xLeftNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
        const int xRightNeighbourCellSerialized = peano4::utils::dLinearised(xRightNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
        const int yLeftNeighbourCellSerialized  = peano4::utils::dLinearised(yLeftNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
        const int yRightNeighbourCellSerialized = peano4::utils::dLinearised(yRightNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);

        const int xLeftYLeftNeighbourCellSerialized = peano4::utils::dLinearised(
          xLeftYLeftNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );

        const int xLeftYRightNeighbourCellSerialized = peano4::utils::dLinearised(
          xLeftYRightNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );

        const int xRightYLeftNeighbourCellSerialized = peano4::utils::dLinearised(
          xRightYLeftNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );

        const int xRightYRightNeighbourCellSerialized = peano4::utils::dLinearised(
          xRightYRightNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );

        const double hCurrentCell = oldQWithHalo[currentCellSerialized * totalVariables + h];
        const double uCurrentCell = (tarch::la::greater(oldQWithHalo[currentCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[currentCellSerialized * totalVariables + hu]/oldQWithHalo[currentCellSerialized * totalVariables + h] : 0.0);
        const double vCurrentCell = (tarch::la::greater(oldQWithHalo[currentCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[currentCellSerialized * totalVariables + hv]/oldQWithHalo[currentCellSerialized * totalVariables + h] : 0.0);
        
        const double hXLeftNeighbourCell = oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h];
        const double uXLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
        const double vXLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[xLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

        const double hXRightNeighbourCell = oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h];
        const double uXRightNeighbourCell = (tarch::la::greater(oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h] : 0.0);
                                           
        const double vXRightNeighbourCell = (tarch::la::greater(oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[xRightNeighbourCellSerialized * totalVariables + h] : 0.0);

        const double hYLeftNeighbourCell = oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h];
        const double uYLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
        const double vYLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[yLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

        const double hYRightNeighbourCell = oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h];
        const double uYRightNeighbourCell = (tarch::la::greater(oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h] : 0.0);
                                           
        const double vYRightNeighbourCell = (tarch::la::greater(oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[yRightNeighbourCellSerialized * totalVariables + h] : 0.0);

        const double uXLeftYLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
        const double vXLeftYLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[xLeftYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);

        const double uXLeftYRightNeighbourCell = (tarch::la::greater(oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h] : 0.0);
                                                 
        const double vXLeftYRightNeighbourCell = (tarch::la::greater(oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[xLeftYRightNeighbourCellSerialized * totalVariables + h] : 0.0);
                                                 

        const double uXRightYLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
                                                
        const double vXRightYLeftNeighbourCell = (tarch::la::greater(oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[xRightYLeftNeighbourCellSerialized * totalVariables + h] : 0.0);
                                                

        const double uXRightYRightNeighbourCell = (tarch::la::greater(oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + hu]/oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h] : 0.0);
                                                  
        const double vXRightYRightNeighbourCell = (tarch::la::greater(oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h],0.0) ? oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + hv]/oldQWithHalo[xRightYRightNeighbourCellSerialized * totalVariables + h] : 0.0);
                                                  


        // h_x
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + h_x]
          = (hCurrentCell - hXLeftNeighbourCell);

        // h_y
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + h_y]
          = (hCurrentCell - hYLeftNeighbourCell);
          
        // u_x
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + u_x]
          = (uCurrentCell - uXLeftNeighbourCell);

        // u_y
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + u_y]
          = (uCurrentCell - uYLeftNeighbourCell);

        // u_xx
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + u_xx]
          = (uXRightNeighbourCell - 2 * uCurrentCell + uXLeftNeighbourCell);

        // u_yy
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + u_yy]
          = (uYRightNeighbourCell - 2 * uCurrentCell + uYLeftNeighbourCell);

        // u_xy
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + u_xy]
          = (uXRightYRightNeighbourCell - uXLeftYRightNeighbourCell - uXRightYLeftNeighbourCell + uXLeftYLeftNeighbourCell);

        // v_x
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + v_x]
          = (vCurrentCell - vXLeftNeighbourCell);

        // v_y
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + v_y]
          = (vCurrentCell - vYLeftNeighbourCell);

        // v_xx
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + v_xx]
          = (vXRightNeighbourCell - 2 * vCurrentCell + vXLeftNeighbourCell);

        // v_yy
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + v_yy]
          = (vYRightNeighbourCell - 2 * vCurrentCell + vYLeftNeighbourCell);

        // v_xy
        oldQWithHalo[currentCellSerialized * totalVariables + NumberOfUnknowns + v_xy]
          = (vXRightYRightNeighbourCell - vXLeftYRightNeighbourCell - vXRightYLeftNeighbourCell + vXLeftYLeftNeighbourCell);
      }
    }
  }
};
