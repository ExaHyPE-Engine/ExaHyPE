#ifndef EXASEIS_CARTESIAN_CONTEXT_HEADER
#define EXASEIS_CARTESIAN_CONTEXT_HEADER

#include "Context.h"
#include "../../../ExaHyPE/kernels/GaussLegendreBasis.h"
#include "../../../../ExaHyPE/kernels/KernelUtils.h"

template <class Shortcuts, int basisSize>
class ContextCartesian: public Context<Shortcuts, basisSize> {

public:
  ContextCartesian(std::string& scenario_string, DomainInformation* info):
    Context<Shortcuts, basisSize>(scenario_string, info) {
    m_info = info;
  };

  ~ContextCartesian() { delete m_info; };

  virtual void initUnknownsPatch(
    double*                                      luh,
    const tarch::la::Vector<Dimensions, double>& center,
    const tarch::la::Vector<Dimensions, double>& dx,
    double                                       t,
    double                                       dt
  ) {

    Shortcuts s;

    constexpr int numberOfData = s.SizeVariables + s.SizeParameters;
    kernels::idx3 id_xyz(basisSize, basisSize, basisSize);
    kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);

    int num_nodes = basisSize;

    double offset_x = center[0] - 0.5 * dx[0];
    double offset_y = center[1] - 0.5 * dx[1];
    double offset_z = center[2] - 0.5 * dx[2];

    double width_x = dx[0];
    double width_y = dx[1];
    double width_z = dx[2];
    std::fill_n(luh, basisSize * basisSize * basisSize * (s.SizeVariables + s.SizeParameters), 0);

    for (int k = 0; k < basisSize; k++) {
      for (int j = 0; j < basisSize; j++) {
        for (int i = 0; i < basisSize; i++) {
          double* Q = luh + id_xyzf(k, j, i, 0);
          double  x[3];
          x[0] = (offset_x + width_x * kernels::legendre::nodes[basisSize - 1][i]);
          x[1] = (offset_y + width_y * kernels::legendre::nodes[basisSize - 1][j]);
          x[2] = (offset_z + width_z * kernels::legendre::nodes[basisSize - 1][k]);
          this->scenario->initUnknownsPointwise(x, center, t, dt, Q);
        }
      }
    }
  };
  DomainInformation* m_info;
};

#endif
