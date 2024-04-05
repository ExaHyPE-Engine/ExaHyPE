// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractNavierStokesSolver.h"
#include "peano4/utils/Loop.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/logging/Log.h"

namespace applications::exahype2::CompressibleNavierStokes {
  enum Coordinates {
    x = 0,
    y,
#if Dimensions == 3
    z
#endif
  };

  enum Variables {
    rho = 0,
    u,
    v,
#if Dimensions == 3
    w,
#endif
    e,
    Z
  };

  enum Derivatives {
    dudx = 0,
    dudy,
#if Dimensions == 3
    dudz,
#endif
    dvdx,
    dvdy,
#if Dimensions == 3
    dvdz,
    dwdx,
    dwdy,
    dwdz,
#endif
    dTdx,
    dTdy
#if Dimensions == 3
    ,
    dTdz
#endif
  };
  class NavierStokesSolver;
} // namespace applications::exahype2::CompressibleNavierStokes

class applications::exahype2::CompressibleNavierStokes::NavierStokesSolver: public AbstractNavierStokesSolver {
private:
  static tarch::logging::Log _log;

public:
  virtual void initialCondition(
    double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    bool                                         gridIsConstructed
  ) override;

  virtual void boundaryConditions(
    const double* __restrict__ Qinside,
    double* __restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    int                                          normal
  ) override;

  virtual ::exahype2::RefinementCommand refinementCriterion(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t
  ) override;

  virtual double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) override;

  static inline double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    Offloadable
  ) {

#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[rho] == Q[0]);
    assertion(Q[rho] > 0);
    assertion(Q[e] > 0);
    assertion((v + normal) < (NumberOfUnknowns + NumberOfAuxiliaryVariables));
    assertion(!std::isnan(Q[rho]));
    assertion(!std::isnan(Q[u]));
    assertion(!std::isnan(Q[v]));
#if Dimensions == 3
    assertion(!std::isnan(Q[w]));
#endif
    assertion(!std::isnan(Q[e]));
#endif

    const double irho = 1.0 / Q[rho];
#if Dimensions == 2
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v]) - Q[Z]);
#else
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) - Q[Z]);
#endif
    assertion(pressure > 0);

    const double c_speedOfSound = std::sqrt(Gamma * pressure / Q[rho]);

    const double max_lambda_convective = std::abs(Q[u + normal]) / Q[rho] + c_speedOfSound;
    const double max_lambda_viscous    = std::max(
      4.0 / 3.0 * Viscosity / Q[rho], Gamma * Viscosity / (PrandtlNumber * Q[rho])
    );
    return std::max(max_lambda_convective, max_lambda_viscous);
  }

  virtual void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  ) override;

  static inline void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F,
    Offloadable
  ) {

#ifdef GPUOffloadingOff
    assertion3(normal >= 0, faceCentre, t, normal);
    assertion3(normal < Dimensions, faceCentre, t, normal);
    assertion(Q[rho] > 0);
    assertion(Q[e] > 0);
    assertion((v + normal) < (NumberOfUnknowns + NumberOfAuxiliaryVariables));
    assertion(!std::isnan(Q[rho]));
    assertion(!std::isnan(Q[u]));
    assertion(!std::isnan(Q[v]));
#if Dimensions == 3
    assertion(!std::isnan(Q[w]));
#endif
    assertion(!std::isnan(Q[e]));
    assertion(!std::isnan(Q[Z]));
#endif

    const double irho = 1.0 / Q[rho];
#if Dimensions == 2
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v]) - Q[Z]);
#else
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * irho * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) - Q[Z]);
#endif

#ifdef GPUOffloadingOff
    assertion(pressure > 0);
#endif

    F[rho] = Q[u + normal];
    F[u]   = irho * Q[u] * Q[u + normal];
    F[v]   = irho * Q[v] * Q[u + normal];
#if Dimensions == 3
    F[w] = irho * Q[w] * Q[u] + normal;
#endif
    F[u + normal] += pressure;
    F[e] = irho * Q[u + normal] * (Q[e] + pressure);
    F[Z] = irho * Q[u + normal] * Q[Z];
  }

  virtual void nonconservativeProduct(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ BTimesDeltaQ
  ) override;

  static inline void nonconservativeProduct(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ BTimesDeltaQ,
    Offloadable
  ) {

#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[rho] > 0);
    assertion(!std::isnan(Q[rho]));
    assertion(!std::isnan(Q[u]));
    assertion(!std::isnan(Q[v]));
    assertion(!std::isnan(Q[e]));
    assertion(!std::isnan(Q[Z]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dudx]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dudy]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dvdx]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dvdy]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dTdx]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dTdy]));
#if Dimensions == 3
    assertion(!std::isnan(Q[w]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dudz]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dvdz]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dwdx]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dwdy]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dwdz]));
    assertion(!std::isnan(Q[NumberOfUnknowns + dTdz]));
#endif
#endif

    double kappa = Viscosity * c_p / PrandtlNumber;
    double c_v   = c_p / Gamma;
    double derivative[NumberOfAuxiliaryVariables];

    auto irho2 = 1.0 / (Q[rho] * Q[rho]);
    auto irho3 = irho2 / Q[rho];
    auto a     = -Q[e] * irho3 + (Q[u] * Q[u] + Q[v] * Q[v]) * irho3;
    auto b     = -Q[u] * irho2;
    auto c     = -Q[v] * irho2;

    if (normal == x) {
      derivative[dudx] = deltaQ[u];
      derivative[dvdx] = deltaQ[v];
      derivative[dudy] = Q[NumberOfUnknowns + dudy] - 0.5 * deltaQ[NumberOfUnknowns + dudy];
      derivative[dvdy] = Q[NumberOfUnknowns + dvdy] - 0.5 * deltaQ[NumberOfUnknowns + dvdy];
      derivative[dTdx]
        = (a * deltaQ[rho] + b * derivative[dudx] + c * derivative[dvdx] - irho2 * (deltaQ[Z] - 2 * Q[Z] * deltaQ[rho]));
      // derivative[dTdx] = Q[NumberOfUnknowns + dTdx] - 0.5 * deltaQ[NumberOfUnknowns + dTdx];

#if Dimensions == 3
      derivative[dwdx] = deltaQ[w];
      derivative[dudz] = Q[NumberOfUnknowns + dudz] - 0.5 * deltaQ[NumberOfUnknowns + dudz];
      derivative[dwdz] = Q[NumberOfUnknowns + dwdz] - 0.5 * deltaQ[NumberOfUnknowns + dwdz];
#endif

      // -mu{2(du/dx) - 2/3 (du/dx + dv/dy)}
      BTimesDeltaQ[u] = -Viscosity * (2.0 * derivative[dudx] - 2.0 / 3.0 * (derivative[dudx] + derivative[dvdy]
#if Dimensions == 3
        +derivative[dwdz]
#endif
        )) / Q[rho];

      // -mu{dv/dx + du/dy}
      BTimesDeltaQ[v] = -Viscosity * (derivative[dvdx] + derivative[dudy]) / Q[rho];

#if Dimensions == 3
      BTimesDeltaQ[w] = -Viscosity * (derivative[dudz] + derivative[dwdx]) / Q[rho];
#endif
    } else {
#if Dimensions == 3
      if (normal == y) {
        derivative[dwdy] = deltaQ[w];
        derivative[dvdz] = Q[NumberOfUnknowns + dvdz] - 0.5 * deltaQ[NumberOfUnknowns + dvdz];
        derivative[dwdz] = Q[NumberOfUnknowns + dwdz] - 0.5 * deltaQ[NumberOfUnknowns + dwdz];
#endif

        derivative[dudy] = deltaQ[u];
        derivative[dvdy] = deltaQ[v];
        derivative[dudx] = Q[NumberOfUnknowns + dudx] - 0.5 * deltaQ[NumberOfUnknowns + dudx];
        derivative[dvdx] = Q[NumberOfUnknowns + dvdx] - 0.5 * deltaQ[NumberOfUnknowns + dvdx];

        derivative[dTdy]
          = (a * deltaQ[rho] + b * derivative[dudy] + c * derivative[dvdy] - irho2 * (deltaQ[Z] - 2 * Q[Z] * deltaQ[rho]));

        // derivative[dTdy] = Q[NumberOfUnknowns + dTdy] - 0.5 * deltaQ[NumberOfUnknowns + dTdy];

        // -mu{dv/dx + du/dy}
        BTimesDeltaQ[u] = -Viscosity * (derivative[dvdx] + derivative[dudy]) / Q[rho];

        // -mu{2(dv/dy) - 2/3 (du/dx + dv/dy)}
        BTimesDeltaQ[v] = -Viscosity * (2.0 * derivative[dvdy] - 2.0 / 3.0 * (derivative[dudx] + derivative[dvdy]
#if Dimensions == 3
        +derivative[dwdz]
#endif
        )) / Q[rho];
#if Dimensions == 3
        BTimesDeltaQ[w] = -Viscosity * (derivative[dvdz] + derivative[dwdy]) / Q[rho];
      } else {

        derivative[dudz] = deltaQ[u];
        derivative[dvdz] = deltaQ[v];
        derivative[dwdz] = deltaQ[w];
        derivative[dudx] = Q[NumberOfUnknowns + dudx] - 0.5 * deltaQ[NumberOfUnknowns + dudx];
        derivative[dvdy] = Q[NumberOfUnknowns + dvdy] - 0.5 * deltaQ[NumberOfUnknowns + dvdy];
        derivative[dudz] = Q[NumberOfUnknowns + dudz] - 0.5 * deltaQ[NumberOfUnknowns + dudz];
        derivative[dwdy] = Q[NumberOfUnknowns + dwdy] - 0.5 * deltaQ[NumberOfUnknowns + dwdy];
        derivative[dwdx] = Q[NumberOfUnknowns + dwdx] - 0.5 * deltaQ[NumberOfUnknowns + dwdx];
        derivative[dTdz] = Q[NumberOfUnknowns + dTdz] - 0.5 * deltaQ[NumberOfUnknowns + dTdz];

        BTimesDeltaQ[u] = -Viscosity * (derivative[dudz] + derivative[dwdx]) / Q[rho];
        BTimesDeltaQ[v] = -Viscosity * (derivative[dvdz] + derivative[dwdy]) / Q[rho];
        BTimesDeltaQ[w] = -Viscosity
                          * (2.0 * derivative[dwdz] - 2.0 / 3.0 * (derivative[dudx] + derivative[dvdy] + derivative[dwdz]))
                          / Q[rho];
      }
#endif
    }
    BTimesDeltaQ[rho] = 0;
    BTimesDeltaQ[Z]   = 0;
#if Dimensions == 2
    BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v]) / Q[rho] - kappa * derivative[dTdx + normal];
#else
    BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v] + Q[w] * BTimesDeltaQ[w]) / Q[rho]
                      - kappa * derivative[dTdx + normal];
#endif
  }


  virtual void sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    double* __restrict__ S
  ) override;

  static inline void extrapolateHalo(double* __restrict__ Q) {
#if Dimensions == 2
    tarch::la::Vector<Dimensions, int> topLeftCell  = {0, NumberOfFiniteVolumesPerAxisPerPatch + 1};
    tarch::la::Vector<Dimensions, int> topRightCell = {
      NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch + 1
    };
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

    const int topLeftCellSerialised = peano4::utils::dLinearised(topLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2);
    const int topRightCellSerialised = peano4::utils::dLinearised(
      topRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomLeftCellSerialised = peano4::utils::dLinearised(
      bottomLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightCellSerialised = peano4::utils::dLinearised(
      bottomRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topLeftCellN1Serialised = peano4::utils::dLinearised(
      topLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topLeftCellN2Serialised = peano4::utils::dLinearised(
      topLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topRightCellN1Serialised = peano4::utils::dLinearised(
      topRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightCellN2Serialised = peano4::utils::dLinearised(
      topRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(
      bottomLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(
      bottomLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomRightCellN1Serialised = peano4::utils::dLinearised(
      bottomRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightCellN2Serialised = peano4::utils::dLinearised(
      bottomRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {

      Q[(topLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (Q[(topLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);
      Q[(topRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (Q[(topRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(bottomLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (Q[(bottomLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(bottomRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (Q[(bottomRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);
    }
#else
    // Edges
    for (int i = 1; i < NumberOfFiniteVolumesPerAxisPerPatch + 1; i++) {
      tarch::la::Vector<Dimensions, int> backLeftCell   = {0, i, 0};
      tarch::la::Vector<Dimensions, int> backLeftCellN1 = {1, i, 0};
      tarch::la::Vector<Dimensions, int> backLeftCellN2 = {0, i, 1};

      const int backLeftCellSerialised = peano4::utils::dLinearised(
        backLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backLeftCellN1Serialised = peano4::utils::dLinearised(
        backLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backLeftCellN2Serialised = peano4::utils::dLinearised(
        backLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> backRightCell   = {NumberOfFiniteVolumesPerAxisPerPatch + 1, i, 0};
      tarch::la::Vector<Dimensions, int> backRightCellN1 = {NumberOfFiniteVolumesPerAxisPerPatch, i, 0};
      tarch::la::Vector<Dimensions, int> backRightCellN2 = {NumberOfFiniteVolumesPerAxisPerPatch + 1, i, 1};

      const int backRightCellSerialised = peano4::utils::dLinearised(
        backRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backRightCellN1Serialised = peano4::utils::dLinearised(
        backRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backRightCellN2Serialised = peano4::utils::dLinearised(
        backRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> backTopCell   = {i, NumberOfFiniteVolumesPerAxisPerPatch + 1, 0};
      tarch::la::Vector<Dimensions, int> backTopCellN1 = {i, NumberOfFiniteVolumesPerAxisPerPatch + 1, 1};
      tarch::la::Vector<Dimensions, int> backTopCellN2 = {i, NumberOfFiniteVolumesPerAxisPerPatch, 0};

      const int backTopCellSerialised = peano4::utils::dLinearised(
        backTopCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backTopCellN1Serialised = peano4::utils::dLinearised(
        backTopCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backTopCellN2Serialised = peano4::utils::dLinearised(
        backTopCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> backBottomCell   = {i, 0, 0};
      tarch::la::Vector<Dimensions, int> backBottomCellN1 = {i, 0, 1};
      tarch::la::Vector<Dimensions, int> backBottomCellN2 = {i, 1, 0};

      const int backBottomCellSerialised = peano4::utils::dLinearised(
        backBottomCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backBottomCellN1Serialised = peano4::utils::dLinearised(
        backBottomCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int backBottomCellN2Serialised = peano4::utils::dLinearised(
        backBottomCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> frontLeftCell   = {0, i, NumberOfFiniteVolumesPerAxisPerPatch + 1};
      tarch::la::Vector<Dimensions, int> frontLeftCellN1 = {1, i, NumberOfFiniteVolumesPerAxisPerPatch + 1};
      tarch::la::Vector<Dimensions, int> frontLeftCellN2 = {0, i, NumberOfFiniteVolumesPerAxisPerPatch};

      const int frontLeftCellSerialised = peano4::utils::dLinearised(
        frontLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontLeftCellN1Serialised = peano4::utils::dLinearised(
        frontLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontLeftCellN2Serialised = peano4::utils::dLinearised(
        frontLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> frontRightCell = {
        NumberOfFiniteVolumesPerAxisPerPatch + 1, i, NumberOfFiniteVolumesPerAxisPerPatch + 1
      };
      tarch::la::Vector<Dimensions, int> frontRightCellN1 = {
        NumberOfFiniteVolumesPerAxisPerPatch, i, NumberOfFiniteVolumesPerAxisPerPatch + 1
      };
      tarch::la::Vector<Dimensions, int> frontRightCellN2 = {
        NumberOfFiniteVolumesPerAxisPerPatch + 1, i, NumberOfFiniteVolumesPerAxisPerPatch
      };

      const int frontRightCellSerialised = peano4::utils::dLinearised(
        frontRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontRightCellN1Serialised = peano4::utils::dLinearised(
        frontRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontRightCellN2Serialised = peano4::utils::dLinearised(
        frontRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> frontTopCell = {
        i, NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch + 1
      };
      tarch::la::Vector<Dimensions, int> frontTopCellN1 = {
        i, NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch
      };
      tarch::la::Vector<Dimensions, int> frontTopCellN2 = {
        i, NumberOfFiniteVolumesPerAxisPerPatch, NumberOfFiniteVolumesPerAxisPerPatch + 1
      };

      const int frontTopCellSerialised = peano4::utils::dLinearised(
        frontTopCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontTopCellN1Serialised = peano4::utils::dLinearised(
        frontTopCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontTopCellN2Serialised = peano4::utils::dLinearised(
        frontTopCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> frontBottomCell   = {i, 0, NumberOfFiniteVolumesPerAxisPerPatch + 1};
      tarch::la::Vector<Dimensions, int> frontBottomCellN1 = {i, 0, NumberOfFiniteVolumesPerAxisPerPatch};
      tarch::la::Vector<Dimensions, int> frontBottomCellN2 = {i, 1, NumberOfFiniteVolumesPerAxisPerPatch + 1};

      const int frontBottomCellSerialised = peano4::utils::dLinearised(
        frontBottomCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontBottomCellN1Serialised = peano4::utils::dLinearised(
        frontBottomCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int frontBottomCellN2Serialised = peano4::utils::dLinearised(
        frontBottomCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> bottomLeftCell   = {0, 0, i};
      tarch::la::Vector<Dimensions, int> bottomLeftCellN1 = {1, 0, i};
      tarch::la::Vector<Dimensions, int> bottomLeftCellN2 = {0, 1, i};

      const int bottomLeftCellSerialised = peano4::utils::dLinearised(
        bottomLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(
        bottomLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(
        bottomLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> bottomRightCell   = {NumberOfFiniteVolumesPerAxisPerPatch + 1, 0, i};
      tarch::la::Vector<Dimensions, int> bottomRightCellN1 = {NumberOfFiniteVolumesPerAxisPerPatch, 0, i};
      tarch::la::Vector<Dimensions, int> bottomRightCellN2 = {NumberOfFiniteVolumesPerAxisPerPatch + 1, 1, i};

      const int bottomRightCellSerialised = peano4::utils::dLinearised(
        bottomRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int bottomRightCellN1Serialised = peano4::utils::dLinearised(
        bottomRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int bottomRightCellN2Serialised = peano4::utils::dLinearised(
        bottomRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> topLeftCell   = {0, NumberOfFiniteVolumesPerAxisPerPatch + 1, i};
      tarch::la::Vector<Dimensions, int> topLeftCellN1 = {1, NumberOfFiniteVolumesPerAxisPerPatch + 1, i};
      tarch::la::Vector<Dimensions, int> topLeftCellN2 = {0, NumberOfFiniteVolumesPerAxisPerPatch, i};

      const int topLeftCellSerialised = peano4::utils::dLinearised(
        topLeftCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int topLeftCellN1Serialised = peano4::utils::dLinearised(
        topLeftCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int topLeftCellN2Serialised = peano4::utils::dLinearised(
        topLeftCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      tarch::la::Vector<Dimensions, int> topRightCell = {
        NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch + 1, i
      };
      tarch::la::Vector<Dimensions, int> topRightCellN1 = {
        NumberOfFiniteVolumesPerAxisPerPatch, NumberOfFiniteVolumesPerAxisPerPatch + 1, i
      };
      tarch::la::Vector<Dimensions, int> topRightCellN2 = {
        NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch, i
      };

      const int topRightCellSerialised = peano4::utils::dLinearised(
        topRightCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int topRightCellN1Serialised = peano4::utils::dLinearised(
        topRightCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );
      const int topRightCellN2Serialised = peano4::utils::dLinearised(
        topRightCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
      );

      for (int j = 0; j < NumberOfAuxiliaryVariables + NumberOfUnknowns; j++) {
        Q[(backLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(backLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(backLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(backRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(backRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(backRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(backTopCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(backTopCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(backTopCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(backBottomCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(backBottomCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(backBottomCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(frontLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(frontLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(frontLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(frontRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(frontRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(frontRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(frontTopCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(frontTopCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(frontTopCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(frontBottomCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(frontBottomCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(frontBottomCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(bottomLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(bottomLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(bottomLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(bottomRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(bottomRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(bottomRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(topLeftCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(topLeftCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(topLeftCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);

        Q[(topRightCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]
          = 0.5
            * (Q[(topRightCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j] + Q[(topRightCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + j]);
      }
    }

    // Corners
    tarch::la::Vector<Dimensions, int> topLeftBackCell  = {0, NumberOfFiniteVolumesPerAxisPerPatch + 1, 0};
    tarch::la::Vector<Dimensions, int> topRightBackCell = {
      NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch + 1, 0
    };
    tarch::la::Vector<Dimensions, int> bottomLeftBackCell  = {0, 0, 0};
    tarch::la::Vector<Dimensions, int> bottomRightBackCell = {NumberOfFiniteVolumesPerAxisPerPatch + 1, 0, 0};

    tarch::la::Vector<Dimensions, int> topLeftFrontCell = {
      0, NumberOfFiniteVolumesPerAxisPerPatch + 1, NumberOfFiniteVolumesPerAxisPerPatch + 1
    };
    tarch::la::Vector<Dimensions, int> topRightFrontCell = {
      NumberOfFiniteVolumesPerAxisPerPatch + 1,
      NumberOfFiniteVolumesPerAxisPerPatch + 1,
      NumberOfFiniteVolumesPerAxisPerPatch + 1
    };
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCell  = {0, 0, NumberOfFiniteVolumesPerAxisPerPatch + 1};
    tarch::la::Vector<Dimensions, int> bottomRightFrontCell = {
      NumberOfFiniteVolumesPerAxisPerPatch + 1, 0, NumberOfFiniteVolumesPerAxisPerPatch + 1
    };

    tarch::la::Vector<Dimensions, int> topLeftBackCellN1 = topLeftBackCell;
    topLeftBackCellN1(0)++;
    tarch::la::Vector<Dimensions, int> topLeftBackCellN2 = topLeftBackCell;
    topLeftBackCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topLeftBackCellN3 = topLeftBackCell;
    topLeftBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> topRightBackCellN1 = topRightBackCell;
    topRightBackCellN1(0)--;
    tarch::la::Vector<Dimensions, int> topRightBackCellN2 = topRightBackCell;
    topRightBackCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topRightBackCellN3 = topRightBackCell;
    topRightBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> bottomLeftBackCellN1 = bottomLeftBackCell;
    bottomLeftBackCellN1(0)++;
    tarch::la::Vector<Dimensions, int> bottomLeftBackCellN2 = bottomLeftBackCell;
    bottomLeftBackCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomLeftBackCellN3 = bottomLeftBackCell;
    bottomLeftBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> bottomRightBackCellN1 = bottomRightBackCell;
    bottomRightBackCellN1(0)--;
    tarch::la::Vector<Dimensions, int> bottomRightBackCellN2 = bottomRightBackCell;
    bottomRightBackCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomRightBackCellN3 = bottomRightBackCell;
    bottomRightBackCellN3(2)++;

    tarch::la::Vector<Dimensions, int> topLeftFrontCellN1 = topLeftFrontCell;
    topLeftFrontCellN1(0)++;
    tarch::la::Vector<Dimensions, int> topLeftFrontCellN2 = topLeftFrontCell;
    topLeftFrontCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topLeftFrontCellN3 = topLeftFrontCell;
    topLeftFrontCellN3(2)--;

    tarch::la::Vector<Dimensions, int> topRightFrontCellN1 = topRightFrontCell;
    topRightFrontCellN1(0)--;
    tarch::la::Vector<Dimensions, int> topRightFrontCellN2 = topRightFrontCell;
    topRightFrontCellN2(1)--;
    tarch::la::Vector<Dimensions, int> topRightFrontCellN3 = topRightFrontCell;
    topRightFrontCellN3(2)--;

    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN1 = {1, 0, NumberOfFiniteVolumesPerAxisPerPatch + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN2 = {0, 1, NumberOfFiniteVolumesPerAxisPerPatch + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN3 = {0, 0, NumberOfFiniteVolumesPerAxisPerPatch};

    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN1 = bottomRightFrontCell;
    bottomRightFrontCellN1(0)--;
    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN2 = bottomRightFrontCell;
    bottomRightFrontCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN3 = bottomRightFrontCell;
    bottomRightFrontCellN3(2)--;

    const int topLeftBackCellSerialised = peano4::utils::dLinearised(
      topLeftBackCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightBackCellSerialised = peano4::utils::dLinearised(
      topRightBackCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftBackCellSerialised = peano4::utils::dLinearised(
      bottomLeftBackCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightBackCellSerialised = peano4::utils::dLinearised(
      bottomRightBackCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topLeftFrontCellSerialised = peano4::utils::dLinearised(
      topLeftFrontCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightFrontCellSerialised = peano4::utils::dLinearised(
      topRightFrontCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftFrontCellSerialised = peano4::utils::dLinearised(
      bottomLeftFrontCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightFrontCellSerialised = peano4::utils::dLinearised(
      bottomRightFrontCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topLeftBackCellN1Serialised = peano4::utils::dLinearised(
      topLeftBackCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topLeftBackCellN2Serialised = peano4::utils::dLinearised(
      topLeftBackCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topLeftBackCellN3Serialised = peano4::utils::dLinearised(
      topLeftBackCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topRightBackCellN1Serialised = peano4::utils::dLinearised(
      topRightBackCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightBackCellN2Serialised = peano4::utils::dLinearised(
      topRightBackCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightBackCellN3Serialised = peano4::utils::dLinearised(
      topRightBackCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomLeftBackCellN1Serialised = peano4::utils::dLinearised(
      bottomLeftBackCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftBackCellN2Serialised = peano4::utils::dLinearised(
      bottomLeftBackCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftBackCellN3Serialised = peano4::utils::dLinearised(
      bottomLeftBackCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomRightBackCellN1Serialised = peano4::utils::dLinearised(
      bottomRightBackCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightBackCellN2Serialised = peano4::utils::dLinearised(
      bottomRightBackCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightBackCellN3Serialised = peano4::utils::dLinearised(
      bottomRightBackCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topLeftFrontCellN1Serialised = peano4::utils::dLinearised(
      topLeftFrontCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topLeftFrontCellN2Serialised = peano4::utils::dLinearised(
      topLeftFrontCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topLeftFrontCellN3Serialised = peano4::utils::dLinearised(
      topLeftFrontCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int topRightFrontCellN1Serialised = peano4::utils::dLinearised(
      topRightFrontCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightFrontCellN2Serialised = peano4::utils::dLinearised(
      topRightFrontCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int topRightFrontCellN3Serialised = peano4::utils::dLinearised(
      topRightFrontCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomLeftFrontCellN1Serialised = peano4::utils::dLinearised(
      bottomLeftFrontCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftFrontCellN2Serialised = peano4::utils::dLinearised(
      bottomLeftFrontCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomLeftFrontCellN3Serialised = peano4::utils::dLinearised(
      bottomLeftFrontCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    const int bottomRightFrontCellN1Serialised = peano4::utils::dLinearised(
      bottomRightFrontCellN1, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightFrontCellN2Serialised = peano4::utils::dLinearised(
      bottomRightFrontCellN2, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );
    const int bottomRightFrontCellN3Serialised = peano4::utils::dLinearised(
      bottomRightFrontCellN3, NumberOfFiniteVolumesPerAxisPerPatch + 2
    );

    for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {

      Q[(topLeftBackCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(topLeftBackCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topLeftBackCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topLeftBackCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(topRightBackCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(topRightBackCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topRightBackCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topRightBackCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(bottomLeftBackCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(bottomLeftBackCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomLeftBackCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomLeftBackCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(bottomRightBackCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(bottomRightBackCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomRightBackCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomRightBackCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(topLeftFrontCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(topLeftFrontCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topLeftFrontCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topLeftFrontCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(topRightFrontCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(topRightFrontCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topRightFrontCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(topRightFrontCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(bottomLeftFrontCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 1.0 / 3.0
          * (Q[(bottomLeftFrontCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomLeftFrontCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomLeftFrontCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);

      Q[(bottomRightFrontCellSerialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]
        = 0.5
          * (Q[(bottomRightFrontCellN1Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomRightFrontCellN2Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i] + Q[(bottomRightFrontCellN3Serialised) * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + i]);
    }
#endif
  }

  static inline void calculateDerivatives(double* __restrict__ Q) {

#if Dimensions == 2

    auto pressure = [](double* __restrict__ Q) {
      return (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho] - Q[Z]);
    };

    for (int x = 0; x < NumberOfFiniteVolumesPerAxisPerPatch + 1; x++)
      for (int y = 0; y < NumberOfFiniteVolumesPerAxisPerPatch + 1; y++) {
        const tarch::la::Vector<Dimensions, int> currentCell    = {x, y};
        const tarch::la::Vector<Dimensions, int> xNeighbourCell = {x + 1, y};
        const tarch::la::Vector<Dimensions, int> yNeighbourCell = {x, y + 1};

        const int currentCellSerialized = peano4::utils::dLinearised(
          currentCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );
        const int xNeighbourCellSerialized = peano4::utils::dLinearised(
          xNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );
        const int yNeighbourCellSerialized = peano4::utils::dLinearised(
          yNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
        );

        // u_x
        Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns]
          = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + u]
            + Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + u];
        // u_y
        Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 1]
          = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1]
            + Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1];
        // v_x
        Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 2]
          = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2]
            + Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2];
        // v_y
        Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 3]
          = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2]
            + Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2];
        // T_x
        Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 4]
          = -pressure(&Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
            ) / (R * Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)])
            + pressure(&Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
              ) / (R * Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]);
        // T_y
        Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 4]
          = -pressure(&Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
            ) / (R * Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)])
            + pressure(&Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
              ) / (R * Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]);
      }
#else

    double pressure = {[](double* __restrict__ Q) {
      return (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) / Q[rho] - Q[Z]);
    }};

    for (int x = 0; x < NumberOfFiniteVolumesPerAxisPerPatch + 1; x++)
      for (int y = 0; y < NumberOfFiniteVolumesPerAxisPerPatch + 1; y++)
        for (int z = 0; z < NumberOfFiniteVolumesPerAxisPerPatch + 1; z++) {
          const tarch::la::Vector<Dimensions, int> currentCell    = {x, y, z};
          const tarch::la::Vector<Dimensions, int> xNeighbourCell = {x + 1, y, z};
          const tarch::la::Vector<Dimensions, int> yNeighbourCell = {x, y + 1, z};
          const tarch::la::Vector<Dimensions, int> zNeighbourCell = {x, y, z + 1};

          const int currentCellSerialized = peano4::utils::dLinearised(
            currentCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
          );
          const int xNeighbourCellSerialized = peano4::utils::dLinearised(
            xNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
          );
          const int yNeighbourCellSerialized = peano4::utils::dLinearised(
            yNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
          );
          const int zNeighbourCellSerialized = peano4::utils::dLinearised(
            zNeighbourCell, NumberOfFiniteVolumesPerAxisPerPatch + 2
          );

          // u_x
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1]
              + Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1];
          // u_y
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 1]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1]
              + Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1];
          // u_z
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 2]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1]
              + Q[zNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 1];
          // v_x
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 3]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2]
              + Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2];
          // v_y
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 4]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2]
              + Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2];
          // v_z
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 5]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2]
              + Q[zNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 2];
          // w_x
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 6]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 3]
              + Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 3];
          // w_y
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 7]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 3]
              + Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 3];
          // w_x
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 8]
            = -Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 3]
              + Q[zNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + 3];
          // T_x
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 9]
            = -pressure(&Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
              ) / (R * Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)])
              + pressure(&Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
                ) / (R * Q[xNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]);
          // T_y
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 10]
            = -pressure(&Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
              ) / (R * Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)])
              + pressure(&Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
                ) / (R * Q[yNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]);
          // T_z
          Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables) + NumberOfUnknowns + 11]
            = -pressure(&Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
              ) / (R * Q[currentCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)])
              + pressure(&Q[zNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]
                ) / (R * Q[zNeighbourCellSerialized * (NumberOfUnknowns + NumberOfAuxiliaryVariables)]);
        }
#endif
  }
};
