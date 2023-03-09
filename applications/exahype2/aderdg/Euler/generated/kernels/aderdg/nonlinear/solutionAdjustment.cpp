
#include "./Kernels.h"
#include "./Quadrature.h"

#include "../../../../euler2D.h"

void generated::kernels::AderDG::nonlinear::solutionAdjustment(
  examples::exahype2::euler2D::euler2D& solver,
  double* luh,
  const double* const center,
  const double dx, //Assume dx[0] == dx[1] == dx[2]
  const double t,
  const double dt
) {
  double x0[2];
  int pos = 0;

    for (int y = 0; y < 4; y++) {
      x0[1] = center[1] + dx * (nodes[y] - 0.5);
      for (int x = 0; x < 4; x++) {
        x0[0] = center[0] + dx * (nodes[x] - 0.5);

        solver.examples::exahype2::euler2D::euler2D::adjustPointSolution(x0, t, dt, luh+(pos*4));
        pos++;
      }
    }

}