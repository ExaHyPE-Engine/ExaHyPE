// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"

#include "tarch/la/Vector.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace benchmarks::exahype2::kernelbenchmarks {
  /**
   * @brief Manages GPU data for a sparse subset of active leaf cells.
   *
   * This repository holds data for fused cells designated for GPU
   * offloading. It is designed to be rebuilt whenever the mesh changes.
   * Data is stored in a Structure-of-Arrays (SoA) layout for coalesced
   * memory access on the GPU.
   *
   * The lifecycle is as follows:
   * 1. startCollection() - Clears old data and prepares for a new build.
   * 2. bookmarkCell() - Called repeatedly during a traversal to collect leaf cells.
   * 3. buildFromBookmarks() - Allocates GPU memory and copies data from host.
   * 4. [COMPUTE ON GPU]
   * 5. syncDeviceToHost() - Copies results from GPU back to the host repository.
   * 6. clear() - Frees all GPU resources.
   */
  class FusedDataRepository {
  public:
    static FusedDataRepository& getInstance();

    FusedDataRepository(const FusedDataRepository&)            = delete;
    FusedDataRepository& operator=(const FusedDataRepository&) = delete;
    FusedDataRepository(FusedDataRepository&&)                 = delete;
    FusedDataRepository& operator=(FusedDataRepository&&)      = delete;

    ~FusedDataRepository();

    void startCollection();

    void bookmarkCell(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize
    );

    void buildFromBookmarks();

    void swapQNewAndQOld();

    void clear();

    void syncCellsDeviceToHost();
    void syncFacesDeviceToHost();

    void syncCellsHostToDevice();
    void syncFacesHostToDevice();

    // --- GPU Data Accessors ---
    double* getQNew();
    double* getQOld();

    uint64_t* getFaceIndices();
    double*   getFaceQNewPool();
    double*   getFaceQOldPool();

    uint64_t* getBoundaryFaceIndices();
    uint8_t*  getBoundaryFaceNumbers();
    uint64_t* getBoundaryCellIndices();

    tarch::la::Vector<DIMENSIONS, double>* getCellCentres();
    tarch::la::Vector<DIMENSIONS, double>* getCellSizes();

    int  getNumberFusedCells() const;
    int  getNumberOfBoundaryFaces() const;
    bool isReady() const;

    double getMinVolumeH() const;
    double getMaxVolumeH() const;

  private:
    FusedDataRepository();

    double* _combinedQBlockDevice = nullptr;

    using FaceKey = std::tuple<int, int, int>;

    double _minVolumeH;
    double _maxVolumeH;

    // --- State Management ---
    bool _isReady;
    int  _numberOfFusedCells;
    int  _numberOfBoundaryFaces;

    // --- GPU Device Pointers ---
    double* _QNewDevice;
    double* _QOldDevice;

    tarch::la::Vector<DIMENSIONS, double>* _cellCentresDevice;
    tarch::la::Vector<DIMENSIONS, double>* _cellSizesDevice;

    // --- Face Data Management ---
    /**
     * @brief A single, contiguous block of memory holding all unique face data.
     */
    double*   _globalFaceQNewPoolDevice;
    double*   _globalFaceQOldPoolDevice;
    uint64_t* _indicesOfFacesDevice;

    uint64_t* _indicesOfBoundaryFacesDevice;
    uint8_t*  _boundaryFaceNumbersDevice;
    uint64_t* _boundaryCellIndicesDevice;

    std::map<FaceKey, uint64_t> _uniqueFaceMap;

    // --- Host-side Bookmarking Data (temporary) ---
    std::vector<tarch::la::Vector<DIMENSIONS, double>> _bookmarkedCellCentres;
    std::vector<tarch::la::Vector<DIMENSIONS, double>> _bookmarkedCellSizes;
  };
} // namespace benchmarks::exahype2::kernelbenchmarks
