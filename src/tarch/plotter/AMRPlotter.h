// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "FileWriter.h"
#include "tarch/la/Vector.h"

namespace tarch::plotter {
  class AMRPlotter;
} // namespace tarch::plotter

class tarch::plotter::AMRPlotter {
public:
  AMRPlotter(FileWriter& plotter, int subdomainId, double scalingFactor = 0.5, double baseThickness = 0.0);
  ~AMRPlotter() = default;

  /**
   * 2D -> 3D Extrusion
   * Plots a 2D cell as a 3D block with variable thickness.
   */
  int plotCell2D(const tarch::la::Vector<2, double>& offset, const tarch::la::Vector<2, double>& size, int level);

  /**
   * 3D -> 3D Shrinking
   * Plots a 3D cell that shrinks as the level increases, creating gaps
   * to visualize the grid structure.
   */
  int plotCell3D(const tarch::la::Vector<3, double>& offset, const tarch::la::Vector<3, double>& size, int level);

private:
  std::unique_ptr<FileWriter::VertexPlotter> _vertexPlotter;
  std::unique_ptr<FileWriter::CellPlotter>   _cellPlotter;

  double _scalingFactor;
  double _baseThickness;
};
