// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "celldata/AderSolverQ.h"
#include "celldata/AderSolverQ_old.h"
#include "facedata/AderSolverQEstimates.h"
#include "facedata/AderSolverQFluxEstimates.h"
#include "peano4/datamanagement/CellMarker.h"
#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace benchmarks::exahype2::kernelbenchmarks {
  /**
   * A singleton repository to manage and provide access to grid cell and face data.
   *
   * This class holds the memory for all cell-local and face-local data arrays
   * (Q, QReconstructed, etc.) for the entire grid. It uses a keying system based
   * on cell position and refinement depth to uniquely identify and retrieve data
   * for any given cell.
   */
  class DataRepository {
  public:
    /**
     * Provides helper functions for calculating linearized array indices from
     * geometric cell positions.
     */
    struct Indexing {
      /**
       * Calculates the linearised index of the lower face of a cell along one axis
       * (along one axis means the face normal is parallel to the axis)
       * formula: faceIndexLow = x_1 + x_2 * (3^Depth)+1 + x_3 * (((3^Depth)+1)*(3^Depth))
       * faceIndexHigh = faceIndexLow + 1
       */
      static int linearise(
        const tarch::la::Vector<DIMENSIONS, int>&    cellCentre,
        const tarch::la::Vector<DIMENSIONS, double>& cellSize
      );

      /**
       * Calculates the indices of the lower faces of a cell along along all axis
       * for x axis: x_1 + x_2 * (3^Depth)+1 + x_3 * (((3^Depth)+1)*(3^Depth))
       * for y axis: x_2 + x_1 * (3^Depth)+1 + x_3 * (((3^Depth)+1)*(3^Depth))
       * for z axis: x_3 + x_1 * (3^Depth)+1 + x_2 * (((3^Depth)+1)*(3^Depth))
       * in 2d (((3^Depth)+1)*(3^Depth)) is not needed, because the faces are only in x and y direction
       * Matrix * Vector: N = (3^Depth)
       *
       * | x_1 x_2 x_3 | | 1 | | index_x |
       * | x_2 x_1 x_3 | * | N+1 | = | index_y |
       * | x_3 x_1 x_2 | | (N+1)*N | | index_z |
       */
      static tarch::la::Vector<DIMENSIONS, int> getFaceIndices(
        const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
        const tarch::la::Vector<DIMENSIONS, double>& cellSize
      );

      static int getFaceIndex(
        const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
        const tarch::la::Vector<DIMENSIONS, double>& cellSize,
        int                                          axis
      );
    };

    static DataRepository& getInstance();

    DataRepository(const DataRepository&)            = delete;
    DataRepository& operator=(const DataRepository&) = delete;
    DataRepository(DataRepository&&)                 = delete;
    DataRepository& operator=(DataRepository&&)      = delete;

    ~DataRepository() = default;

    SolverStoragePrecision* getCellQ(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize
    );

    SolverStoragePrecision* getcellQOld(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize
    );

    SolverCorrectorPrecision* getFaceQEstimates(int index, int depth, int axis);
    SolverCorrectorPrecision* getFaceQEstimates(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      int                                          axis
    );

    SolverCorrectorPrecision* getFaceQFluxEstimates(int index, int depth, int axis);
    SolverCorrectorPrecision* getFaceQFluxEstimates(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      int                                          axis
    );

    int getFaceEstimatesCardinality() const;
    int getFaceFluxEstimatesCardinality() const;
    // QReconstructed is not stored explicitly anymore, as it is just used to store the patch+halo temporarily before
    // the Riemann step but we still need its size, therefore the getter.
    int getCellQCardinality() const;
    int getcellQOldCardinality() const;

    std::size_t getMemoryUsageInBytes() const;

  private:
    using CellQ              = std::array<SolverStoragePrecision, celldata::AderSolverQ::Cardinality>;
    using cellQOld           = std::array<SolverStoragePrecision, celldata::AderSolverQ_old::Cardinality>;
    using FaceQEstimates     = std::array<SolverCorrectorPrecision, facedata::AderSolverQEstimates::Cardinality>;
    using FaceQFluxEstimates = std::array<SolverCorrectorPrecision, facedata::AderSolverQFluxEstimates::Cardinality>;

    struct DepthLevel {
      std::vector<CellQ>    Q;
      std::vector<cellQOld> QReconstructed;

      // Stores faces based on linearised order (face GridSize = 3^Depth * (3^Depth) + 1)
      std::array<std::vector<FaceQEstimates>, DIMENSIONS>     faceEstimates;
      std::array<std::vector<FaceQFluxEstimates>, DIMENSIONS> faceFluxes;
    };

    std::vector<DepthLevel> _depthLevels;

    DataRepository();
  };
} // namespace benchmarks::exahype2::kernelbenchmarks
