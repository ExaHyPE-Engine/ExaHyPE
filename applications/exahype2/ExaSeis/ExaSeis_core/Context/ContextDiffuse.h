#ifndef EXASEIS_CONTEXT_DIFFUSE_HEADER
#define EXASEIS_CONTEXT_DIFFUSE_HEADER

#include "Context.h"
#include "../Context/DomainInformation.h"
#include "../Context/SolverInformationFV.h"
#include "../Diffuse/DiffuseInterface.h"
#include "../Topography/TopographyFactory.h"
#include "../../../ExaHyPE/kernels/GaussLegendreBasis.h"
#include "../../../../ExaHyPE/kernels/KernelUtils.h"

template <class Shortcuts, int basisSize>
class ContextDiffuse: public Context<Shortcuts, basisSize> {

public:
  ContextDiffuse(
    std::string&       scenario_string,
    std::string&       topography_string,
    DomainInformation* a_domainInfo,
    SolverInformation* a_solverInfo,
    double             a_interface_factor,
    double             a_max_distance_to_direct_surface
  ):
    Context<Shortcuts, basisSize>(scenario_string, a_domainInfo) {

    domainInfo             = a_domainInfo;
    solverInfo             = a_solverInfo;
    Topography* topography = TopographyFactory::createTopography(topography_string, a_domainInfo);
    interface              = new DiffuseInterface<Shortcuts, basisSize>(
      topography, a_domainInfo, a_solverInfo, a_interface_factor, a_max_distance_to_direct_surface
    );
  };
  ~ContextDiffuse() {
    delete interface;
    delete domainInfo;
    delete solverInfo;
  };

  virtual void initUnknownsPatch(
    double*                                      luh,
    const tarch::la::Vector<Dimensions, double>& center,
    const tarch::la::Vector<Dimensions, double>& dx,
    double                                       t,
    double                                       dt
  ) override {
    Shortcuts s;
    int       numberOfData = s.SizeVariables + s.SizeParameters;

    kernels::idx3 id_xyz(basisSize, basisSize, basisSize);
    kernels::idx4 id_xyzn(basisSize, basisSize, basisSize, Dimensions);
    kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);

    double offset_x = center[0] - 0.5 * dx[0];
    double offset_y = center[1] - 0.5 * dx[1];
    double offset_z = center[2] - 0.5 * dx[2];

    double width_x = dx[0];
    double width_y = dx[1];
    double width_z = dx[2];

    double nodes[basisSize * basisSize * basisSize * Dimensions];
    double alpha[basisSize * basisSize * basisSize];
    double quad_nodes[basisSize];

    if (solverInfo->isDG()) {
      std::copy_n(kernels::legendre::nodes[basisSize - 1], basisSize, quad_nodes);
    } else {
      for (int i = 1; i < basisSize + 1; i++) {
        quad_nodes[i - 1] = i / static_cast<double>(basisSize + 1);
      }
    }

    for (int k = 0; k < basisSize; k++) {
      double z = (offset_z + width_z * quad_nodes[k]);
      for (int j = 0; j < basisSize; j++) {
        double y = (offset_y + width_y * quad_nodes[j]);
        for (int i = 0; i < basisSize; i++) {
          double x                   = (offset_x + width_x * quad_nodes[i]);
          nodes[id_xyzn(k, j, i, 0)] = x;
          nodes[id_xyzn(k, j, i, 1)] = y;
          nodes[id_xyzn(k, j, i, 2)] = z;
        }
      }
    }

    interface->getAlphaPatch(nodes, alpha);

    for (int k = 0; k < basisSize; k++) {
      for (int j = 0; j < basisSize; j++) {
        for (int i = 0; i < basisSize; i++) {
          luh[id_xyzf(k, j, i, s.alpha)] = alpha[id_xyz(k, j, i)];
          this->scenario->initUnknownsPointwise(nodes + id_xyzn(k, j, i, 0), center, t, dt, luh + id_xyzf(k, j, i, 0));
          assertion(luh[id_xyzf(k, j, i, s.rho)] > 0.0);
        }
      }
    }
#ifdef Asserts
    for (int k = 0; k < basisSize; k++) {
      for (int j = 0; j < basisSize; j++) {
        for (int i = 0; i < basisSize; i++) {
          for (int m = 0; m < numberOfData; m++) {
            assertion5(std::isfinite(luh[id_xyzf(k, j, i, m)]), k, j, i, m, domainInfo->meshLevel);
          }
          assertion(luh[id_xyzf(k, j, i, s.rho)] > 0.0);
        }
      }
    }
#endif
  };

  void initUnknownsPointwise(const double* const x, const double t, const double dt, double* Q) {
    Shortcuts s;
    const int numberOfData = s.SizeVariables + s.SizeParameters;
    Q[s.alpha]             = interface->getAlpha(x);
    tarch::la::Vector<Dimensions, double> center;
    center[0] = x[0];
    center[1] = x[1];
    center[2] = x[2];

    this->scenario->initUnknownsPointwise(x, center, t, dt, Q);
#ifdef Asserts
    for (int m = 0; m < numberOfData; m++) {
      assertion2(std::isfinite(Q[m]), m, domainInfo->meshLevel);
    }
#endif
  };

private:
  DomainInformation*                      domainInfo;
  SolverInformation*                      solverInfo;
  DiffuseInterface<Shortcuts, basisSize>* interface;
};

#endif
