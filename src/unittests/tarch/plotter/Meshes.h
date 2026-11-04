// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::plotter {
  struct Mesh;
} // namespace tarch::plotter

namespace tarch::plotter::unittests {
  void appendUnitSquares(tarch::plotter::Mesh& mesh, double x, int numberOfSquares);
  void appendUnitCube(tarch::plotter::Mesh& mesh, double x);
} // namespace tarch::plotter::unittests
