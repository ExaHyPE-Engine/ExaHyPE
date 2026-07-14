// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FusedDataRepository.h"

#include "DataRepository.h"
#include "tarch/accelerator/accelerator.h"

namespace benchmarks::exahype2::kernelbenchmarks {
  FusedDataRepository& FusedDataRepository::getInstance() {
    static FusedDataRepository instance;
    return instance;
  }

  FusedDataRepository::FusedDataRepository():
    _combinedQBlockDevice(nullptr),
    _minVolumeH(0.0),
    _maxVolumeH(0.0),
    _isReady(false),
    _numberOfFusedCells(0),
    _numberOfBoundaryFaces(0),
    _QNewDevice(nullptr),
    _QOldDevice(nullptr),
    _cellCentresDevice(nullptr),
    _cellSizesDevice(nullptr),
    _globalFaceQNewPoolDevice(nullptr),
    _globalFaceQOldPoolDevice(nullptr),
    _indicesOfFacesDevice(nullptr),
    _indicesOfBoundaryFacesDevice(nullptr),
    _boundaryFaceNumbersDevice(nullptr),
    _boundaryCellIndicesDevice(nullptr) {}

  FusedDataRepository::~FusedDataRepository() { clear(); }

  void FusedDataRepository::startCollection() {
    _bookmarkedCellCentres.clear();
    _bookmarkedCellSizes.clear();
    _uniqueFaceMap.clear();
    _isReady            = false;
    _numberOfFusedCells = 0;
    _minVolumeH         = std::numeric_limits<double>::max();
    _maxVolumeH         = std::numeric_limits<double>::lowest();
  }

  void FusedDataRepository::bookmarkCell(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    _bookmarkedCellCentres.push_back(cellCentre);
    _bookmarkedCellSizes.push_back(cellSize);
  }

  void FusedDataRepository::buildFromBookmarks() {
    clear();
    _uniqueFaceMap.clear();

    if (_bookmarkedCellCentres.empty()) {
      return;
    }

    _minVolumeH                     = std::numeric_limits<double>::max();
    _maxVolumeH                     = std::numeric_limits<double>::lowest();
    _numberOfFusedCells             = _bookmarkedCellCentres.size();
    DataRepository& hostRepo        = DataRepository::getInstance();
    const auto      qCardinality    = hostRepo.getCellCardinality();
    const auto      faceCardinality = hostRepo.getFaceCardinality();

    // --- 1. Allocate non-face GPU Memory ---
    const size_t qSize      = static_cast<size_t>(_numberOfFusedCells) * static_cast<size_t>(qCardinality);
    const size_t totalQSize = qSize * 2;

    _combinedQBlockDevice = tarch::allocateMemory<double>(totalQSize, tarch::MemoryLocation::Device);
    _QNewDevice           = _combinedQBlockDevice;
    _QOldDevice           = _combinedQBlockDevice + qSize;

    // --- 2. Copy non-face data directly (per-cell) ---
    for (int i = 0; i < _numberOfFusedCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      _minVolumeH            = std::min(_minVolumeH, tarch::la::min(cellSize));
      _maxVolumeH            = std::max(_maxVolumeH, tarch::la::max(cellSize));

      tarch::copyTo(
        _QNewDevice + i * qCardinality,
        hostRepo.getCellQNew(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
      tarch::copyTo(
        _QOldDevice + i * qCardinality,
        hostRepo.getCellQOld(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
    }

    // --- 3. Build Unique Face Pool on Host ---
    // This map will store a unique key for each face and map it to its dense index (0, 1, 2...)
    // in our new global pool.
    std::vector<double>   hostGlobalFacePool;
    std::vector<uint64_t> indicesOfBoundaryFacesIntoGlobalFacePool;
    std::vector<uint8_t>  hostBoundaryFaceNumbers;
    std::vector<uint64_t> hostBoundaryCellIndices;
    hostGlobalFacePool.reserve(_numberOfFusedCells * 2 * faceCardinality); // Pre-allocate rough estimate

    for (int i = 0; i < _numberOfFusedCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      for (int f = 0; f < TWO_TIMES_D; ++f) {
        // 1. Calculate Key to check uniqueness
        const double ratio   = DomainSize[0] / cellSize[0];
        const int    depth   = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
        FaceKey      faceKey = std::
          make_tuple(DataRepository::Indexing::getFaceIndex(cellCentre, cellSize, f), depth, f % DIMENSIONS);

        // 2. Find or Insert into Map
        uint64_t globalFaceIndex;
        if (_uniqueFaceMap.find(faceKey) == _uniqueFaceMap.end()) {
          // New face
          globalFaceIndex         = _uniqueFaceMap.size();
          _uniqueFaceMap[faceKey] = globalFaceIndex;

          double* srcFaceData = hostRepo.getFaceQNew(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey));
          hostGlobalFacePool.insert(hostGlobalFacePool.end(), srcFaceData, srcFaceData + faceCardinality);
        } else {
          // Existing face
          globalFaceIndex = _uniqueFaceMap[faceKey];
        }

        // 3. Check if Boundary Face
        tarch::la::Vector<DIMENSIONS, double> faceCentre(cellCentre);
        int                                   normal = f % DIMENSIONS;
        faceCentre(normal) += f >= DIMENSIONS ? cellSize(normal) / 2.0 : -cellSize(normal) / 2.0;
        bool isBoundary = false;
        isBoundary |= tarch::la::equals(faceCentre(normal), DomainOffset(normal));
        isBoundary |= tarch::la::equals(faceCentre(normal), DomainOffset(normal) + DomainSize(normal));

        if (isBoundary) {
          indicesOfBoundaryFacesIntoGlobalFacePool.push_back(globalFaceIndex);
          hostBoundaryFaceNumbers.push_back(f);
          hostBoundaryCellIndices.push_back(i);
        }
      }
    }

    // --- 4. Allocate and Copy Global Face Pool to Device ---
    _globalFaceQNewPoolDevice = tarch::allocateMemory<double>(hostGlobalFacePool.size(), tarch::MemoryLocation::Device);
    _globalFaceQOldPoolDevice = tarch::allocateMemory<double>(hostGlobalFacePool.size(), tarch::MemoryLocation::Device);
    _indicesOfBoundaryFacesDevice = tarch::allocateMemory<uint64_t>(
      indicesOfBoundaryFacesIntoGlobalFacePool.size(),
      tarch::MemoryLocation::Device
    );
    _boundaryFaceNumbersDevice = tarch::allocateMemory<uint8_t>(
      hostBoundaryFaceNumbers.size(),
      tarch::MemoryLocation::Device
    );
    _boundaryCellIndicesDevice = tarch::allocateMemory<uint64_t>(
      hostBoundaryCellIndices.size(),
      tarch::MemoryLocation::Device
    );

    tarch::copyTo(
      _globalFaceQNewPoolDevice,
      hostGlobalFacePool.data(),
      hostGlobalFacePool.size(),
      tarch::MemoryLocation::Device
    );

    // --- Copy boundary faces to Device
    tarch::copyTo(
      _indicesOfBoundaryFacesDevice,
      indicesOfBoundaryFacesIntoGlobalFacePool.data(),
      indicesOfBoundaryFacesIntoGlobalFacePool.size(),
      tarch::MemoryLocation::Device
    );
    tarch::copyTo(
      _boundaryFaceNumbersDevice,
      hostBoundaryFaceNumbers.data(),
      hostBoundaryFaceNumbers.size(),
      tarch::MemoryLocation::Device
    );
    tarch::copyTo(
      _boundaryCellIndicesDevice,
      hostBoundaryCellIndices.data(),
      hostBoundaryCellIndices.size(),
      tarch::MemoryLocation::Device
    );
    _numberOfBoundaryFaces = indicesOfBoundaryFacesIntoGlobalFacePool.size();

    // --- 5. Build Host-side Face Index Array ---
    uint64_t* indicesOfFacesHost = tarch::allocateMemory<uint64_t>(
      _numberOfFusedCells * TWO_TIMES_D,
      tarch::MemoryLocation::Heap
    );
    _indicesOfFacesDevice = tarch::allocateMemory<uint64_t>(
      _numberOfFusedCells * TWO_TIMES_D,
      tarch::MemoryLocation::Device
    );

    for (int i = 0; i < _numberOfFusedCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      for (int f = 0; f < TWO_TIMES_D; ++f) {
        // faceIndex, axis
        const double ratio   = DomainSize[0] / cellSize[0];
        const int    depth   = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
        FaceKey      faceKey = std::
          make_tuple(DataRepository::Indexing::getFaceIndex(cellCentre, cellSize, f), depth, f % DIMENSIONS);
        uint64_t globalFaceIndex                = _uniqueFaceMap.at(faceKey);
        indicesOfFacesHost[f + i * TWO_TIMES_D] = globalFaceIndex;
      }
    }

    tarch::copyTo(
      _indicesOfFacesDevice,
      indicesOfFacesHost,
      _numberOfFusedCells * TWO_TIMES_D,
      tarch::MemoryLocation::Device
    );

    // --- 6. Copy Geometric Data (can be done in bulk) ---
    _cellCentresDevice = tarch::allocateMemory<tarch::la::Vector<DIMENSIONS, double>>(
      _numberOfFusedCells,
      tarch::MemoryLocation::Device
    );
    _cellSizesDevice = tarch::allocateMemory<tarch::la::Vector<DIMENSIONS, double>>(
      _numberOfFusedCells,
      tarch::MemoryLocation::Device
    );
    tarch::copyTo(
      _cellCentresDevice,
      _bookmarkedCellCentres.data(),
      _bookmarkedCellCentres.size(),
      tarch::MemoryLocation::Device
    );
    tarch::
      copyTo(_cellSizesDevice, _bookmarkedCellSizes.data(), _bookmarkedCellSizes.size(), tarch::MemoryLocation::Device);

    tarch::freeMemory(indicesOfFacesHost, tarch::MemoryLocation::Heap);

    _isReady = true;
  }

  void FusedDataRepository::swapQNewAndQOld() {
    if (!_isReady) {
      return;
    }
    std::swap(_QNewDevice, _QOldDevice);
    std::swap(_globalFaceQNewPoolDevice, _globalFaceQOldPoolDevice);
  }

  void FusedDataRepository::clear() {
    tarch::freeMemory(_combinedQBlockDevice, tarch::MemoryLocation::Device);
    _combinedQBlockDevice = nullptr;

    tarch::freeMemory(_indicesOfBoundaryFacesDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_indicesOfFacesDevice, tarch::MemoryLocation::Device);

    tarch::freeMemory(_cellCentresDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_cellSizesDevice, tarch::MemoryLocation::Device);

    // Clear face data
    tarch::freeMemory(_globalFaceQNewPoolDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_globalFaceQOldPoolDevice, tarch::MemoryLocation::Device);

    // Clear boundary data
    tarch::freeMemory(_boundaryFaceNumbersDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_boundaryCellIndicesDevice, tarch::MemoryLocation::Device);

    _QNewDevice               = nullptr;
    _QOldDevice               = nullptr;
    _cellCentresDevice        = nullptr;
    _cellSizesDevice          = nullptr;
    _globalFaceQNewPoolDevice = nullptr;
    _globalFaceQOldPoolDevice = nullptr;

    _indicesOfBoundaryFacesDevice = nullptr;
    _indicesOfFacesDevice         = nullptr;
    _boundaryFaceNumbersDevice    = nullptr;
    _boundaryCellIndicesDevice    = nullptr;
    _isReady                      = false;
    _numberOfFusedCells           = 0;
    _numberOfBoundaryFaces        = 0;
  }

  void FusedDataRepository::syncCellsDeviceToHost() {
    if (!_isReady) {
      return;
    }

    DataRepository& hostRepo     = DataRepository::getInstance();
    const auto      qCardinality = hostRepo.getCellCardinality();

    for (int i = 0; i < _numberOfFusedCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      tarch::copyFrom(
        _QNewDevice + i * qCardinality,
        hostRepo.getCellQNew(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  void FusedDataRepository::syncFacesDeviceToHost() {
    if (!_isReady) {
      return;
    }

    DataRepository& hostRepo        = DataRepository::getInstance();
    const auto      faceCardinality = hostRepo.getFaceCardinality();

    for (auto face : _uniqueFaceMap) {
      // faceIndex, axis
      FaceKey  faceKey         = face.first;
      uint64_t globalFaceIndex = face.second;
      tarch::copyFrom(
        _globalFaceQNewPoolDevice + globalFaceIndex * faceCardinality,
        hostRepo.getFaceQNew(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  void FusedDataRepository::syncCellsHostToDevice() {
    if (!_isReady) {
      return;
    }

    DataRepository& hostRepo     = DataRepository::getInstance();
    const auto      qCardinality = hostRepo.getCellCardinality();

    for (int i = 0; i < _numberOfFusedCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      tarch::copyTo(
        _QNewDevice + i * qCardinality,
        hostRepo.getCellQNew(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  void FusedDataRepository::syncFacesHostToDevice() {
    if (!_isReady) {
      return;
    }

    const auto      faceCardinality = DataRepository::getInstance().getFaceCardinality();
    DataRepository& hostRepo        = DataRepository::getInstance();
    for (auto face : _uniqueFaceMap) {
      // faceIndex, axis
      FaceKey  faceKey         = face.first;
      uint64_t globalFaceIndex = face.second;
      tarch::copyTo(
        _globalFaceQNewPoolDevice + globalFaceIndex * faceCardinality,
        hostRepo.getFaceQNew(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  // --- GPU Data Accessors ---
  double*   FusedDataRepository::getQNew() { return _QNewDevice; }
  double*   FusedDataRepository::getQOld() { return _QOldDevice; }
  double*   FusedDataRepository::getFaceQNewPool() { return _globalFaceQNewPoolDevice; }
  double*   FusedDataRepository::getFaceQOldPool() { return _globalFaceQOldPoolDevice; }
  uint64_t* FusedDataRepository::getFaceIndices() { return _indicesOfFacesDevice; }
  uint64_t* FusedDataRepository::getBoundaryFaceIndices() { return _indicesOfBoundaryFacesDevice; }
  uint8_t*  FusedDataRepository::getBoundaryFaceNumbers() { return _boundaryFaceNumbersDevice; }
  uint64_t* FusedDataRepository::getBoundaryCellIndices() { return _boundaryCellIndicesDevice; }
  tarch::la::Vector<DIMENSIONS, double>* FusedDataRepository::getCellCentres() { return _cellCentresDevice; }
  tarch::la::Vector<DIMENSIONS, double>* FusedDataRepository::getCellSizes() { return _cellSizesDevice; }
  int    FusedDataRepository::getNumberFusedCells() const { return _numberOfFusedCells; }
  int    FusedDataRepository::getNumberOfBoundaryFaces() const { return _numberOfBoundaryFaces; }
  bool   FusedDataRepository::isReady() const { return _isReady; }
  double FusedDataRepository::getMinVolumeH() const { return _minVolumeH; }
  double FusedDataRepository::getMaxVolumeH() const { return _maxVolumeH; }
} // namespace benchmarks::exahype2::kernelbenchmarks
