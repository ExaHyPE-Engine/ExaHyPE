// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractNavierStokesSolver.h"
#include "exahype2/fv/PatchAccess.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/logging/Log.h"
#include "peano4/utils/Loop.h"

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
    e
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
    bool                                         gridIsConstructred
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

#if Dimensions == 2
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho]);
#else
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) / Q[rho]);
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
    assertion(normal >= 0);
    assertion(normal < Dimensions);
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

#if Dimensions == 2
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v]) / Q[rho]);
#else
    const double pressure = (Gamma - 1) * (Q[e] - 0.5 * (Q[u] * Q[u] + Q[v] * Q[v] + Q[w] * Q[w]) / Q[rho]);
#endif

#ifdef GPUOffloadingOff
    assertion(pressure > 0);
#endif

    F[rho] = Q[u + normal];
    F[e]   = Q[u + normal] * (Q[e] + pressure) / Q[rho];

    if (normal == 0) {
      F[u] = Q[u] * Q[u] / Q[rho] + pressure;
      F[v] = Q[v] * Q[v] / Q[rho];
#if Dimensions == 3
      F[w] = Q[w] * Q[u] / Q[rho];
#endif
    } else {
#if Dimensions == 3
      if (normal == 1) {
#endif
        F[u] = Q[u] * Q[v] / Q[rho];
        F[v] = Q[v] * Q[v] / Q[rho] + pressure;
#if Dimensions == 3
        F[w] = Q[w] * Q[v] / Q[rho];
      } else {
        F[u] = Q[u] * Q[w] / Q[rho];
        F[v] = Q[v] * Q[w] / Q[rho];
        F[w] = Q[w] * Q[w] / Q[rho] + pressure;
      }
#endif
    }
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
#if Dimensions == 3
    assertion(!std::isnan(Q[w]));
#endif
#endif

    ::exahype2::fv::PatchAccess access(
      Q,                                    // make it initially point to input
      NumberOfFiniteVolumesPerAxisPerPatch, // defined in superclass
      1,                                    // halo size, which you know as user
      NumberOfUnknowns,                     // defined in superclass
      NumberOfAuxiliaryVariables            // defined in superclass
    );

    if (normal == x) {
      // Centre (Self) values
      const double u_c = Q[u];
      const double v_c = Q[v];

      // Top neighbour values
      const double u_t = access.right(y, u);
      const double v_t = access.right(y, v);

      // Deriavtives
      const double u_y = u_t - u_c;
      const double v_y = v_t - v_c;

      const double u_x = deltaQ[u];
      const double v_x = deltaQ[v];

#if Dimensions == 3
      const double w_c = Q[w];
      const double w_t = access.right(y, w);

      // Front neighbour values
      const double u_f = access.right(z, u);
      const double w_f = access.right(z, w);

      // Deriavtives
      const double u_z = u_f - u_c;
      const double w_z = w_f - w_c;

      const double w_x = deltaQ[w];
#endif
      // -mu{2(du/dx) - 2/3 (du/dx + dv/dy)}
      BTimesDeltaQ[u] = -Viscosity * (2.0 * u_x - 2.0 / 3.0 * (u_x + v_y
#if Dimension == 3
      +w_z
#endif
      )) / Q[rho];

      // -mu{dv/dx + du/dy}
      BTimesDeltaQ[v] = -Viscosity * (v_x + u_y) / Q[rho];

#if Dimension == 3
      BTimesDeltaQ[w] = -Viscosity * (u_z + w_x) / Q[rho];
#endif

    } else

#if Dimensions == 2
    {
#else
      if (normal == y) {
#endif
      // Self values
      const double u_c = Q[u];
      const double v_c = Q[v];

      // Right neighbour values
      const double u_r = access.right(x, u);
      const double v_r = access.right(x, v);

      // Deriavtives
      const double u_x = u_r - u_c;
      const double v_x = v_r - v_c;

      const double u_y = deltaQ[u];
      const double v_y = deltaQ[v];

#if Dimensions == 3
      const double w_c = Q[w];
      const double w_r = access.right(x, w);

      // Front neighbour values
      const double v_f = access.right(z, v);
      const double w_f = access.right(z, w);

      // Deriavtives
      const double v_z = v_f - v_c;
      const double w_z = w_f - w_c;

      const double w_y = deltaQ[w];
#endif

      // -mu{dv/dx + du/dy}
      BTimesDeltaQ[u] = -Viscosity * (v_x + u_y) / Q[rho];

      // -mu{2(dv/dy) - 2/3 (du/dx + dv/dy)}
      BTimesDeltaQ[v] = -Viscosity * (2.0 * v_y- 2.0 / 3.0 * (u_x + v_y
#if Dimension == 3
      + w_z
#endif
      )) / Q[rho];
#if Dimension == 3
      BTimesDeltaQ[w] = -Viscosity * (v_z + w_y) / Q[rho];
    }
    else {
      // Self values
      const double u_c = Q[u];
      const double v_c = Q[v];
      const double w_c = Q[w];

      // Right neighbour values
      const double u_r = access.right(x, u);
      const double v_r = access.right(x, v);
      const double w_r = access.right(x, w);

      // Top neighbour values
      const double u_t = access.right(y, u);
      const double v_t = access.right(y, v);
      const double w_t = access.right(y, w);

      // Deriavtives
      const double u_x = u_r - u_c;
      const double w_x = w_r - w_c;

      const double v_y = v_t - v_c;
      const double w_y = w_t - w_c;

      const double u_z = deltaQ[u];
      const double v_z = deltaQ[v];
      const double w_z = deltaQ[w];

      BTimesDeltaQ[u] = -Viscosity * (u_z + w_x) / Q[rho];
      BTimesDeltaQ[v] = -Viscosity * (v_z + w_y) / Q[rho];
      BTimesDeltaQ[w] = -Viscosity * (2.0 * w_z - 2.0 / 3.0 * (u_x + v_y + w_z)) / Q[rho];
#endif
    }

    BTimesDeltaQ[rho] = 0;
#if Dimensions == 2
    BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v]) / Q[rho];
#else
    BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v] + Q[w] * BTimesDeltaQ[w]) / Q[rho];
#endif
  }

  static void extrapolateHalo(double* __restrict__ oldQWithHalo ){
    int patchSize = NumberOfFiniteVolumesPerAxisPerPatch;
    int aux_var = NumberOfAuxiliaryVariables;
    int real_var = NumberOfUnknowns;

    #if Dimensions == 2
    tarch::la::Vector<Dimensions, int> topLeftCell     = {0, patchSize + 1};
    tarch::la::Vector<Dimensions, int> topRightCell    = {patchSize + 1, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftCell  = {0, 0};
    tarch::la::Vector<Dimensions, int> bottomRightCell = {patchSize + 1, 0};

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

    const int topLeftCellSerialised     = peano4::utils::dLinearised(topLeftCell, patchSize + 2 * 1);
    const int topRightCellSerialised    = peano4::utils::dLinearised(topRightCell, patchSize + 2 * 1);
    const int bottomLeftCellSerialised  = peano4::utils::dLinearised(bottomLeftCell, patchSize + 2 * 1);
    const int bottomRightCellSerialised = peano4::utils::dLinearised(bottomRightCell, patchSize + 2 * 1);

    const int topLeftCellN1Serialised = peano4::utils::dLinearised(topLeftCellN1, patchSize + 2 * 1);
    const int topLeftCellN2Serialised = peano4::utils::dLinearised(topLeftCellN2, patchSize + 2 * 1);

    const int topRightCellN1Serialised = peano4::utils::dLinearised(topRightCellN1, patchSize + 2 * 1);
    const int topRightCellN2Serialised = peano4::utils::dLinearised(topRightCellN2, patchSize + 2 * 1);

    const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(bottomLeftCellN1, patchSize + 2 * 1);
    const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(bottomLeftCellN2, patchSize + 2 * 1);

    const int bottomRightCellN1Serialised = peano4::utils::dLinearised(bottomRightCellN1, patchSize + 2 * 1);
    const int bottomRightCellN2Serialised = peano4::utils::dLinearised(bottomRightCellN2, patchSize + 2 * 1);

    for (int i = 0; i < real_var + aux_var; i++) {

      oldQWithHalo[(topLeftCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(topLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftCellN2Serialised) * (real_var + aux_var) + i]);
      oldQWithHalo[(topRightCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(topRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightCellN2Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomLeftCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomLeftCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftCellN2Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomRightCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomRightCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightCellN2Serialised) * (real_var + aux_var) + i]);
    }
#else
    // Edges
    for (int i = 1; i < patchSize + 1; i++) {
      tarch::la::Vector<Dimensions, int> backLeftCell   = {0, i, 0};
      tarch::la::Vector<Dimensions, int> backLeftCellN1 = {1, i, 0};
      tarch::la::Vector<Dimensions, int> backLeftCellN2 = {0, i, 1};

      const int backLeftCellSerialised   = peano4::utils::dLinearised(backLeftCell, patchSize + 2 * 1);
      const int backLeftCellN1Serialised = peano4::utils::dLinearised(backLeftCellN1, patchSize + 2 * 1);
      const int backLeftCellN2Serialised = peano4::utils::dLinearised(backLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> backRightCell   = {patchSize + 1, i, 0};
      tarch::la::Vector<Dimensions, int> backRightCellN1 = {patchSize, i, 0};
      tarch::la::Vector<Dimensions, int> backRightCellN2 = {patchSize + 1, i, 1};

      const int backRightCellSerialised   = peano4::utils::dLinearised(backRightCell, patchSize + 2 * 1);
      const int backRightCellN1Serialised = peano4::utils::dLinearised(backRightCellN1, patchSize + 2 * 1);
      const int backRightCellN2Serialised = peano4::utils::dLinearised(backRightCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> backTopCell   = {i, patchSize + 1, 0};
      tarch::la::Vector<Dimensions, int> backTopCellN1 = {i, patchSize + 1, 1};
      tarch::la::Vector<Dimensions, int> backTopCellN2 = {i, patchSize, 0};

      const int backTopCellSerialised   = peano4::utils::dLinearised(backTopCell, patchSize + 2 * 1);
      const int backTopCellN1Serialised = peano4::utils::dLinearised(backTopCellN1, patchSize + 2 * 1);
      const int backTopCellN2Serialised = peano4::utils::dLinearised(backTopCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> backBottomCell   = {i, 0, 0};
      tarch::la::Vector<Dimensions, int> backBottomCellN1 = {i, 0, 1};
      tarch::la::Vector<Dimensions, int> backBottomCellN2 = {i, 1, 0};

      const int backBottomCellSerialised   = peano4::utils::dLinearised(backBottomCell, patchSize + 2 * 1);
      const int backBottomCellN1Serialised = peano4::utils::dLinearised(backBottomCellN1, patchSize + 2 * 1);
      const int backBottomCellN2Serialised = peano4::utils::dLinearised(backBottomCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontLeftCell   = {0, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontLeftCellN1 = {1, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontLeftCellN2 = {0, i, patchSize};

      const int frontLeftCellSerialised   = peano4::utils::dLinearised(frontLeftCell, patchSize + 2 * 1);
      const int frontLeftCellN1Serialised = peano4::utils::dLinearised(frontLeftCellN1, patchSize + 2 * 1);
      const int frontLeftCellN2Serialised = peano4::utils::dLinearised(frontLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontRightCell   = {patchSize + 1, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontRightCellN1 = {patchSize, i, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontRightCellN2 = {patchSize + 1, i, patchSize};

      const int frontRightCellSerialised   = peano4::utils::dLinearised(frontRightCell, patchSize + 2 * 1);
      const int frontRightCellN1Serialised = peano4::utils::dLinearised(frontRightCellN1, patchSize + 2 * 1);
      const int frontRightCellN2Serialised = peano4::utils::dLinearised(frontRightCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontTopCell   = {i, patchSize + 1, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontTopCellN1 = {i, patchSize + 1, patchSize};
      tarch::la::Vector<Dimensions, int> frontTopCellN2 = {i, patchSize, patchSize + 1};

      const int frontTopCellSerialised   = peano4::utils::dLinearised(frontTopCell, patchSize + 2 * 1);
      const int frontTopCellN1Serialised = peano4::utils::dLinearised(frontTopCellN1, patchSize + 2 * 1);
      const int frontTopCellN2Serialised = peano4::utils::dLinearised(frontTopCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> frontBottomCell   = {i, 0, patchSize + 1};
      tarch::la::Vector<Dimensions, int> frontBottomCellN1 = {i, 0, patchSize};
      tarch::la::Vector<Dimensions, int> frontBottomCellN2 = {i, 1, patchSize + 1};

      const int frontBottomCellSerialised   = peano4::utils::dLinearised(frontBottomCell, patchSize + 2 * 1);
      const int frontBottomCellN1Serialised = peano4::utils::dLinearised(frontBottomCellN1, patchSize + 2 * 1);
      const int frontBottomCellN2Serialised = peano4::utils::dLinearised(frontBottomCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> bottomLeftCell   = {0, 0, i};
      tarch::la::Vector<Dimensions, int> bottomLeftCellN1 = {1, 0, i};
      tarch::la::Vector<Dimensions, int> bottomLeftCellN2 = {0, 1, i};

      const int bottomLeftCellSerialised   = peano4::utils::dLinearised(bottomLeftCell, patchSize + 2 * 1);
      const int bottomLeftCellN1Serialised = peano4::utils::dLinearised(bottomLeftCellN1, patchSize + 2 * 1);
      const int bottomLeftCellN2Serialised = peano4::utils::dLinearised(bottomLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> bottomRightCell   = {patchSize + 1, 0, i};
      tarch::la::Vector<Dimensions, int> bottomRightCellN1 = {patchSize, 0, i};
      tarch::la::Vector<Dimensions, int> bottomRightCellN2 = {patchSize + 1, 1, i};

      const int bottomRightCellSerialised   = peano4::utils::dLinearised(bottomRightCell, patchSize + 2 * 1);
      const int bottomRightCellN1Serialised = peano4::utils::dLinearised(bottomRightCellN1, patchSize + 2 * 1);
      const int bottomRightCellN2Serialised = peano4::utils::dLinearised(bottomRightCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> topLeftCell   = {0, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topLeftCellN1 = {1, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topLeftCellN2 = {0, patchSize, i};

      const int topLeftCellSerialised   = peano4::utils::dLinearised(topLeftCell, patchSize + 2 * 1);
      const int topLeftCellN1Serialised = peano4::utils::dLinearised(topLeftCellN1, patchSize + 2 * 1);
      const int topLeftCellN2Serialised = peano4::utils::dLinearised(topLeftCellN2, patchSize + 2 * 1);

      tarch::la::Vector<Dimensions, int> topRightCell   = {patchSize + 1, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topRightCellN1 = {patchSize, patchSize + 1, i};
      tarch::la::Vector<Dimensions, int> topRightCellN2 = {patchSize + 1, patchSize, i};

      const int topRightCellSerialised   = peano4::utils::dLinearised(topRightCell, patchSize + 2 * 1);
      const int topRightCellN1Serialised = peano4::utils::dLinearised(topRightCellN1, patchSize + 2 * 1);
      const int topRightCellN2Serialised = peano4::utils::dLinearised(topRightCellN2, patchSize + 2 * 1);

      for (int j = 0; j < aux_var + real_var; j++) {
        oldQWithHalo[(backLeftCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(backLeftCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(backLeftCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(backRightCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(backRightCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(backRightCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(backTopCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(backTopCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(backTopCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(backBottomCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(backBottomCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(backBottomCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(frontLeftCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(frontLeftCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(frontLeftCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(frontRightCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(frontRightCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(frontRightCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(frontTopCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(frontTopCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(frontTopCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(frontBottomCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(frontBottomCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(frontBottomCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(bottomLeftCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(bottomLeftCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(bottomLeftCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(bottomRightCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(bottomRightCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(bottomRightCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(topLeftCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(topLeftCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(topLeftCellN2Serialised) * (real_var + aux_var) + j]);

        oldQWithHalo[(topRightCellSerialised) * (real_var + aux_var) + j]
          = 0.5
            * (oldQWithHalo[(topRightCellN1Serialised) * (real_var + aux_var) + j] + oldQWithHalo[(topRightCellN2Serialised) * (real_var + aux_var) + j]);
      }
    }

    // Corners
    tarch::la::Vector<Dimensions, int> topLeftBackCell     = {0, patchSize + 1, 0};
    tarch::la::Vector<Dimensions, int> topRightBackCell    = {patchSize + 1, patchSize + 1, 0};
    tarch::la::Vector<Dimensions, int> bottomLeftBackCell  = {0, 0, 0};
    tarch::la::Vector<Dimensions, int> bottomRightBackCell = {patchSize + 1, 0, 0};

    tarch::la::Vector<Dimensions, int> topLeftFrontCell     = {0, patchSize + 1, patchSize + 1};
    tarch::la::Vector<Dimensions, int> topRightFrontCell    = {patchSize + 1, patchSize + 1, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCell  = {0, 0, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomRightFrontCell = {patchSize + 1, 0, patchSize + 1};

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

    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN1 = {1, 0, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN2 = {0, 1, patchSize + 1};
    tarch::la::Vector<Dimensions, int> bottomLeftFrontCellN3 = {0, 0, patchSize};

    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN1 = bottomRightFrontCell;
    bottomRightFrontCellN1(0)--;
    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN2 = bottomRightFrontCell;
    bottomRightFrontCellN2(1)++;
    tarch::la::Vector<Dimensions, int> bottomRightFrontCellN3 = bottomRightFrontCell;
    bottomRightFrontCellN3(2)--;

    const int topLeftBackCellSerialised     = peano4::utils::dLinearised(topLeftBackCell, patchSize + 2 * 1);
    const int topRightBackCellSerialised    = peano4::utils::dLinearised(topRightBackCell, patchSize + 2 * 1);
    const int bottomLeftBackCellSerialised  = peano4::utils::dLinearised(bottomLeftBackCell, patchSize + 2 * 1);
    const int bottomRightBackCellSerialised = peano4::utils::dLinearised(bottomRightBackCell, patchSize + 2 * 1);

    const int topLeftFrontCellSerialised     = peano4::utils::dLinearised(topLeftFrontCell, patchSize + 2 * 1);
    const int topRightFrontCellSerialised    = peano4::utils::dLinearised(topRightFrontCell, patchSize + 2 * 1);
    const int bottomLeftFrontCellSerialised  = peano4::utils::dLinearised(bottomLeftFrontCell, patchSize + 2 * 1);
    const int bottomRightFrontCellSerialised = peano4::utils::dLinearised(bottomRightFrontCell, patchSize + 2 * 1);

    const int topLeftBackCellN1Serialised = peano4::utils::dLinearised(topLeftBackCellN1, patchSize + 2 * 1);
    const int topLeftBackCellN2Serialised = peano4::utils::dLinearised(topLeftBackCellN2, patchSize + 2 * 1);
    const int topLeftBackCellN3Serialised = peano4::utils::dLinearised(topLeftBackCellN3, patchSize + 2 * 1);

    const int topRightBackCellN1Serialised = peano4::utils::dLinearised(topRightBackCellN1, patchSize + 2 * 1);
    const int topRightBackCellN2Serialised = peano4::utils::dLinearised(topRightBackCellN2, patchSize + 2 * 1);
    const int topRightBackCellN3Serialised = peano4::utils::dLinearised(topRightBackCellN3, patchSize + 2 * 1);

    const int bottomLeftBackCellN1Serialised = peano4::utils::dLinearised(bottomLeftBackCellN1, patchSize + 2 * 1);
    const int bottomLeftBackCellN2Serialised = peano4::utils::dLinearised(bottomLeftBackCellN2, patchSize + 2 * 1);
    const int bottomLeftBackCellN3Serialised = peano4::utils::dLinearised(bottomLeftBackCellN3, patchSize + 2 * 1);

    const int bottomRightBackCellN1Serialised = peano4::utils::dLinearised(bottomRightBackCellN1, patchSize + 2 * 1);
    const int bottomRightBackCellN2Serialised = peano4::utils::dLinearised(bottomRightBackCellN2, patchSize + 2 * 1);
    const int bottomRightBackCellN3Serialised = peano4::utils::dLinearised(bottomRightBackCellN3, patchSize + 2 * 1);

    const int topLeftFrontCellN1Serialised = peano4::utils::dLinearised(topLeftFrontCellN1, patchSize + 2 * 1);
    const int topLeftFrontCellN2Serialised = peano4::utils::dLinearised(topLeftFrontCellN2, patchSize + 2 * 1);
    const int topLeftFrontCellN3Serialised = peano4::utils::dLinearised(topLeftFrontCellN3, patchSize + 2 * 1);

    const int topRightFrontCellN1Serialised = peano4::utils::dLinearised(topRightFrontCellN1, patchSize + 2 * 1);
    const int topRightFrontCellN2Serialised = peano4::utils::dLinearised(topRightFrontCellN2, patchSize + 2 * 1);
    const int topRightFrontCellN3Serialised = peano4::utils::dLinearised(topRightFrontCellN3, patchSize + 2 * 1);

    const int bottomLeftFrontCellN1Serialised = peano4::utils::dLinearised(bottomLeftFrontCellN1, patchSize + 2 * 1);
    const int bottomLeftFrontCellN2Serialised = peano4::utils::dLinearised(bottomLeftFrontCellN2, patchSize + 2 * 1);
    const int bottomLeftFrontCellN3Serialised = peano4::utils::dLinearised(bottomLeftFrontCellN3, patchSize + 2 * 1);

    const int bottomRightFrontCellN1Serialised = peano4::utils::dLinearised(bottomRightFrontCellN1, patchSize + 2 * 1);
    const int bottomRightFrontCellN2Serialised = peano4::utils::dLinearised(bottomRightFrontCellN2, patchSize + 2 * 1);
    const int bottomRightFrontCellN3Serialised = peano4::utils::dLinearised(bottomRightFrontCellN3, patchSize + 2 * 1);

    for (int i = 0; i < real_var + aux_var; i++) {

      oldQWithHalo[(topLeftBackCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(topLeftBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(topRightBackCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(topRightBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomLeftBackCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(bottomLeftBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomRightBackCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(bottomRightBackCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightBackCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightBackCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(topLeftFrontCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(topLeftFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topLeftFrontCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(topRightFrontCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(topRightFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(topRightFrontCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomLeftFrontCellSerialised) * (real_var + aux_var) + i]
        = 1.0 / 3.0
          * (oldQWithHalo[(bottomLeftFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomLeftFrontCellN3Serialised) * (real_var + aux_var) + i]);

      oldQWithHalo[(bottomRightFrontCellSerialised) * (real_var + aux_var) + i]
        = 0.5
          * (oldQWithHalo[(bottomRightFrontCellN1Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightFrontCellN2Serialised) * (real_var + aux_var) + i] + oldQWithHalo[(bottomRightFrontCellN3Serialised) * (real_var + aux_var) + i]);
    }
#endif
  }
};
