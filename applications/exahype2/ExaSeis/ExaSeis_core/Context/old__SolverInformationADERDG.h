#ifndef EXASEIS_SOLVERADERDG_INFORMATION_HEADER
#define EXASEIS_SOLVERADERDG_INFORMATION_HEADER

#include <algorithm>

#include "SolverInformation.h"
#include "kernels/GaussLegendreBasis.h"
#include "exahype/solvers/ADERDGSolver.h"
#include "../../PML_LOH1_Marc_test/AbstractElasticWaveSolver.h"

template <int order>
class SolverInformationADERDG: public SolverInformation {
public:
  //  SolverInformationADERDG(exahype::solvers::ADERDGSolver* a_solver): SolverInformation(a_solver){
  SolverInformationADERDG(Elastic::AbstractElasticWaveSolver* a_solver):
    SolverInformation(a_solver) {

    std::copy_n(a_solver->nodes[order], order + 1, nodes);
    for (int i = 0; i < order + 1; i++) {
      std::copy_n(a_solver->dudx[order][i], order + 1, dudx[i]);
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
