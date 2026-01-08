// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AMRPlotter.h"

#include <cmath>
#include <vector>

tarch::plotter::AMRPlotter::
  AMRPlotter(FileWriter& plotter, int subdomainId, double scalingFactor, double baseThickness):
  _vertexPlotter(plotter.createVertexPlotter(subdomainId)),
  _cellPlotter(plotter.createCellPlotter(subdomainId)),
  _scalingFactor(scalingFactor),
  _baseThickness(baseThickness) {}

int tarch::plotter::AMRPlotter::
  plotCell2D(const tarch::la::Vector<2, double>& offset, const tarch::la::Vector<2, double>& size, int level) {
  if (_baseThickness == 0.0) {
    int idx[4];
    idx[0] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1), 0.0});
    idx[1] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1), 0.0});
    idx[2] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1) + size(1), 0.0});
    idx[3] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1) + size(1), 0.0});

    return _cellPlotter->plotQuadrangle(idx);
  }

  // Decay thickness: base * (scale ^ level)
  double thickness = _baseThickness * std::pow(_scalingFactor, level);

  double zBottom = -thickness / 2.0;
  double zTop    = thickness / 2.0;

  int idx[8];
  // Bottom Face
  idx[0] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1), zBottom});
  idx[1] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1), zBottom});
  idx[2] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1) + size(1), zBottom});
  idx[3] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1) + size(1), zBottom});

  // Top Face
  idx[4] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1), zTop});
  idx[5] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1), zTop});
  idx[6] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1) + size(1), zTop});
  idx[7] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1) + size(1), zTop});

  return _cellPlotter->plotHexahedron(idx);
}

int tarch::plotter::AMRPlotter::
  plotCell3D(const tarch::la::Vector<3, double>& offset, const tarch::la::Vector<3, double>& size, int level) {
  int idx[8];

  // Back Face (z = offset(2))
  idx[0] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1), offset(2)});
  idx[1] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0) + size(0), offset(1), offset(2)});
  idx[2] = _vertexPlotter->plotVertex(
    tarch::la::Vector<3, double>{offset(0) + size(0), offset(1) + size(1), offset(2)}
  );
  idx[3] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1) + size(1), offset(2)});

  // Front Face (z = offset(2) + size(2))
  idx[4] = _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{offset(0), offset(1), offset(2) + size(2)});
  idx[5] = _vertexPlotter->plotVertex(
    tarch::la::Vector<3, double>{offset(0) + size(0), offset(1), offset(2) + size(2)}
  );
  idx[6] = _vertexPlotter->plotVertex(
    tarch::la::Vector<3, double>{offset(0) + size(0), offset(1) + size(1), offset(2) + size(2)}
  );
  idx[7] = _vertexPlotter->plotVertex(
    tarch::la::Vector<3, double>{offset(0), offset(1) + size(1), offset(2) + size(2)}
  );

  return _cellPlotter->plotHexahedron(idx);
}
