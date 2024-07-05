#ifndef EXASEIS_SOLVERADERDG_INFORMATION_HEADER
#define EXASEIS_SOLVERADERDG_INFORMATION_HEADER

#include <algorithm>
#include "SolverInformation.h"


template <int order>
class SolverInformationADERDG: public SolverInformation {
public:
  SolverInformationADERDG(exahype2::Solver* a_solver, double* _nodes, double* _dudx):
    SolverInformation(a_solver) {

    const int ndof = order+1;
    //Invert order because kernelgenerator stores these in opposite order
    for(int i=0; i<ndof; i++){
      nodes[i] = _nodes[order-i];
      for(int j=0; j<ndof; j++){
        dudx[i][j] = _dudx[(order-j)*ndof+(order-i)];
      }
    }

#ifdef Asserts
    for (int i = 0; i < order + 1; i++) {
      assertion2(std::isfinite(nodes[i]), nodes[i], i);
    }

    for (int i = 0; i < order + 1; i++) {
      for (int j = 0; j < order + 1; j++) {
        assertion3(std::isfinite(dudx[i][j]), dudx[i][j], i, j);
      }
    }
#endif
  };

  double getNodes(int i) { return nodes[i]; }

  double getDuDx(int i, int j) { return dudx[i][j]; }

  bool isDG() override { return true; }

  double nodes[order + 1];
  double dudx[order + 1][order + 1];
};
#endif
