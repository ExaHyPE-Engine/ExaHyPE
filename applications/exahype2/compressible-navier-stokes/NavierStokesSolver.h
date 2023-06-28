// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractNavierStokesSolver.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/logging/Log.h"

namespace applications::exahype2::CompressibleNavierStokes
{
  enum Coordinates
  {
    x = 0,
    y,
#if Dimensions == 3
    z
#endif
  };

  enum Variables
  {
    rho = 0,
    u,
    v,
#if Dimensions == 3
    w,
#endif
    e
  };

  enum Derivatives
  {
    drhodx = 0,
    drhody,
#if Dimensions == 3
    drhodz,
#endif
    dudx,
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
    dedx,
    dedy,
#if Dimensions == 3
    dedz
#endif
  };

  class NavierStokesSolver;
} // namespace applications::exahype2::CompressibleNavierStokes

class applications::exahype2::CompressibleNavierStokes::NavierStokesSolver: public AbstractNavierStokesSolver
{
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
  )
  {
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
    // std::cout << Q[rho] << "," << Q[u] << "," << Q[v] << "," << Q[w] << "," << Q[e] << "\n";
    // std::cout << faceCentre[0] << "," << faceCentre[1] << "," << faceCentre[2] << "\n";
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
  )
  {
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

    if (normal == 0)
    {
      F[u] = Q[u] * Q[u] / Q[rho] + pressure;
      F[v] = Q[v] * Q[v] / Q[rho];
#if Dimensions == 3
      F[w] = Q[w] * Q[u] / Q[rho];
#endif
    }
    else
    {
#if Dimensions == 3
      if (normal == 1)
      {
#endif
        F[u] = Q[u] * Q[v] / Q[rho];
        F[v] = Q[v] * Q[v] / Q[rho] + pressure;
#if Dimensions == 3
        F[w] = Q[w] * Q[v] / Q[rho];
      }
      else
      {
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
  )
  {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[rho] > 0);
    assertion(!std::isnan(Q[rho]));
    assertion(!std::isnan(Q[u]));
    assertion(!std::isnan(Q[v]));
    assertion(!std::isnan(Q[e]));
    assertion(!std::isnan(deltaQ[drhodx]));
    assertion(!std::isnan(deltaQ[drhody]));
    assertion(!std::isnan(deltaQ[dudx]));
    assertion(!std::isnan(deltaQ[dudy]));
    assertion(!std::isnan(deltaQ[dvdx]));
    assertion(!std::isnan(deltaQ[dvdy]));
    assertion(!std::isnan(deltaQ[dedx]));
    assertion(!std::isnan(deltaQ[dedy]));
#if Dimensions == 3
    assertion(!std::isnan(Q[w]));
    assertion(!std::isnan(Q[drhodz]));
    assertion(!std::isnan(deltaQ[dudz]));
    assertion(!std::isnan(deltaQ[dvdz]));
    assertion(!std::isnan(deltaQ[dwdx]));
    assertion(!std::isnan(deltaQ[dwdy]));
    assertion(!std::isnan(deltaQ[dwdz]));
    assertion(!std::isnan(deltaQ[dedz]));
#endif
#endif

    if (normal == 0)
    {
      // -mu{2(du/dx) - 2/3 (du/dx + dv/dy)}
      BTimesDeltaQ[u] = -Viscosity * (2.0 * deltaQ[dudx] - 2.0 / 3.0 * (deltaQ[dudx] + deltaQ[dvdy]
#if Dimension == 3
      +deltaQ[dwdz]
#endif
      )) / Q[rho];

      // -mu{dv/dx + du/dy}
      BTimesDeltaQ[v] = -Viscosity * (deltaQ[dvdx] + deltaQ[dudy]) / Q[rho];

#if Dimension == 3
      BTimesDeltaQ[w] = -Viscosity * (deltaQ[dudz] + deltaQ[dwdx]) / Q[rho];
#endif
    }
    else
    {
#if Dimension == 3
      if (normal == 1)
      {
#endif
        // -mu{dv/dx + du/dy}
        BTimesDeltaQ[u] = -Viscosity * (deltaQ[dvdx] + deltaQ[dudy]) / Q[rho];

        // -mu{2(dv/dy) - 2/3 (du/dx + dv/dy)}
        BTimesDeltaQ[v] = -Viscosity * (2.0 * deltaQ[dvdy] - 2.0 / 3.0 * (deltaQ[dudx] + deltaQ[dvdy]
#if Dimension == 3
      +deltaQ[dwdz]
#endif
      )) / Q[rho];
#if Dimension == 3
        BTimesDeltaQ[w] = -Viscosity * (deltaQ[dvdz] + deltaQ[dwdy]) / Q[rho];
      }
      else
      {
        BTimesDeltaQ[u] = -Viscosity * (deltaQ[dudz] + deltaQ[dwdx]) / Q[rho];
        BTimesDeltaQ[v] = -Viscosity * (deltaQ[dvdz] + deltaQ[dwdy]) / Q[rho];
        BTimesDeltaQ[w] = -Viscosity * (2.0 * deltaQ[dwdz] - 2.0 / 3.0 * (deltaQ[dudx] + deltaQ[dvdy] + deltaQ[dwdz]))
                          / Q[rho];
      }
#endif
    }

    BTimesDeltaQ[rho] = 0;
#if Dimensions == 2
    BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v]) / Q[rho];
#else
    BTimesDeltaQ[e] = (Q[u] * BTimesDeltaQ[u] + Q[v] * BTimesDeltaQ[v] + Q[w] * BTimesDeltaQ[w]) / Q[rho];
#endif
  }
};