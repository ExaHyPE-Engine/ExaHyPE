// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveDataRepository.h"

#include "DataRepository.h"
#include "tarch/accelerator/accelerator.h"

namespace benchmarks::exahype2::kernelbenchmarks {
  EnclaveDataRepository& EnclaveDataRepository::getInstance() {
    static EnclaveDataRepository instance;
    return instance;
  }

  EnclaveDataRepository::EnclaveDataRepository():
    _isReady(false),
    _numberOfEnclaveCells(0),
    _QDevice(nullptr),
    _QReconstructedDevice(nullptr),
    _cellCentresDevice(nullptr),
    _cellSizesDevice(nullptr),
    _globalFacePoolDevice(nullptr) {}

  EnclaveDataRepository::~EnclaveDataRepository() { clear(); }

  void EnclaveDataRepository::startCollection() {
    _bookmarkedCellCentres.clear();
    _bookmarkedCellSizes.clear();
    _uniqueFaceMap.clear();
    _isReady              = false;
    _numberOfEnclaveCells = 0;
    _minVolumeH           = std::numeric_limits<double>::max();
    _maxVolumeH           = std::numeric_limits<double>::min();
  }

  void EnclaveDataRepository::bookmarkCell(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    _bookmarkedCellCentres.push_back(cellCentre);
    _bookmarkedCellSizes.push_back(cellSize);
  }

  void EnclaveDataRepository::buildFromBookmarks() {
    if (_bookmarkedCellCentres.empty()) {
      return;
    }

    clear();

    _numberOfEnclaveCells                     = _bookmarkedCellCentres.size();
    DataRepository& hostRepo                  = DataRepository::getInstance();
    const auto      qCardinality              = hostRepo.getCellQCardinality();
    const auto      qReconstructedCardinality = hostRepo.getCellQReconstructedCardinality();
    const auto      faceCardinality           = hostRepo.getFaceCardinality();

    // --- 1. Allocate non-face GPU Memory ---
    const size_t qSize              = static_cast<size_t>(_numberOfEnclaveCells) * static_cast<size_t>(qCardinality);
    const size_t qReconstructedSize = static_cast<size_t>(_numberOfEnclaveCells)
                                      * static_cast<size_t>(qReconstructedCardinality);
    const size_t totalQSize = qSize + qReconstructedSize;

    _combinedQBlockDevice = tarch::allocateMemory<double>(totalQSize, tarch::MemoryLocation::Device);
    _QDevice              = _combinedQBlockDevice;
    _QReconstructedDevice = _combinedQBlockDevice + qSize;

    // --- 2. Copy non-face data directly (per-cell) ---
    for (int i = 0; i < _numberOfEnclaveCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      _minVolumeH            = std::min(_minVolumeH, tarch::la::min(cellSize));
      _maxVolumeH            = std::max(_maxVolumeH, tarch::la::max(cellSize));

      tarch::copyTo(
        _QDevice + i * qCardinality,
        hostRepo.getCellQ(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
      tarch::copyTo(
        _QReconstructedDevice + i * qReconstructedCardinality,
        hostRepo.getCellQReconstructed(cellCentre, cellSize),
        qReconstructedCardinality,
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
    hostGlobalFacePool.reserve(_numberOfEnclaveCells * 2 * faceCardinality); // Pre-allocate rough estimate

    for (int i = 0; i < _numberOfEnclaveCells; ++i) {
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
    _globalFacePoolDevice = tarch::allocateMemory<double>(hostGlobalFacePool.size(), tarch::MemoryLocation::Device);
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
      _globalFacePoolDevice,
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
      _numberOfEnclaveCells * TWO_TIMES_D,
      tarch::MemoryLocation::Heap
    );
    _indicesOfFacesDevice = tarch::allocateMemory<uint64_t>(
      _numberOfEnclaveCells * TWO_TIMES_D,
      tarch::MemoryLocation::Device
    );

    for (int i = 0; i < _numberOfEnclaveCells; ++i) {
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
      _numberOfEnclaveCells * TWO_TIMES_D,
      tarch::MemoryLocation::Device
    );

    // --- 6. Copy Geometric Data (can be done in bulk) ---
    _cellCentresDevice = tarch::allocateMemory<tarch::la::Vector<DIMENSIONS, double>>(
      _numberOfEnclaveCells,
      tarch::MemoryLocation::Device
    );
    _cellSizesDevice = tarch::allocateMemory<tarch::la::Vector<DIMENSIONS, double>>(
      _numberOfEnclaveCells,
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

  void EnclaveDataRepository::clear() {
    tarch::freeMemory(_combinedQBlockDevice, tarch::MemoryLocation::Device);
    _combinedQBlockDevice = nullptr;

    tarch::freeMemory(_indicesOfBoundaryFacesDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_indicesOfFacesDevice, tarch::MemoryLocation::Device);

    tarch::freeMemory(_cellCentresDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_cellSizesDevice, tarch::MemoryLocation::Device);

    // Clear face data
    tarch::freeMemory(_globalFacePoolDevice, tarch::MemoryLocation::Device);

    // Clear boundary data
    tarch::freeMemory(_boundaryFaceNumbersDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_boundaryCellIndicesDevice, tarch::MemoryLocation::Device);

    _QDevice              = nullptr;
    _QReconstructedDevice = nullptr;
    _cellCentresDevice    = nullptr;
    _cellSizesDevice      = nullptr;
    _globalFacePoolDevice = nullptr;

    _indicesOfBoundaryFacesDevice = nullptr;
    _indicesOfFacesDevice         = nullptr;
    _boundaryFaceNumbersDevice    = nullptr;
    _boundaryCellIndicesDevice    = nullptr;
    _isReady                      = false;
    _numberOfEnclaveCells         = 0;
    _numberOfBoundaryFaces        = 0;
  }

  void EnclaveDataRepository::syncCellsDeviceToHost() {
    if (!_isReady) {
      return;
    }

    DataRepository& hostRepo     = DataRepository::getInstance();
    const auto      qCardinality = hostRepo.getCellQCardinality();

    for (int i = 0; i < _numberOfEnclaveCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      tarch::copyFrom(
        _QDevice + i * qCardinality,
        hostRepo.getCellQ(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  void EnclaveDataRepository::syncFacesDeviceToHost() {
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
        _globalFacePoolDevice + globalFaceIndex * faceCardinality,
        hostRepo.getFaceQNew(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  void EnclaveDataRepository::syncCellsHostToDevice() {
    if (!_isReady) {
      return;
    }

    DataRepository& hostRepo     = DataRepository::getInstance();
    const auto      qCardinality = hostRepo.getCellQCardinality();

    for (int i = 0; i < _numberOfEnclaveCells; ++i) {
      const auto& cellCentre = _bookmarkedCellCentres[i];
      const auto& cellSize   = _bookmarkedCellSizes[i];
      tarch::copyTo(
        _QDevice + i * qCardinality,
        hostRepo.getCellQ(cellCentre, cellSize),
        qCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  void EnclaveDataRepository::syncFacesHostToDevice() {
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
        _globalFacePoolDevice + globalFaceIndex * faceCardinality,
        hostRepo.getFaceQNew(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  // --- GPU Data Accessors ---
  double*   EnclaveDataRepository::getQ() { return _QDevice; }
  double*   EnclaveDataRepository::getQReconstructed() { return _QReconstructedDevice; }
  double*   EnclaveDataRepository::getFacePool() { return _globalFacePoolDevice; }
  uint64_t* EnclaveDataRepository::getFaceIndices() { return _indicesOfFacesDevice; }
  uint64_t* EnclaveDataRepository::getBoundaryFaceIndices() { return _indicesOfBoundaryFacesDevice; }
  uint8_t*  EnclaveDataRepository::getBoundaryFaceNumbers() { return _boundaryFaceNumbersDevice; }
  uint64_t* EnclaveDataRepository::getBoundaryCellIndices() { return _boundaryCellIndicesDevice; }
  tarch::la::Vector<DIMENSIONS, double>* EnclaveDataRepository::getCellCentres() { return _cellCentresDevice; }
  tarch::la::Vector<DIMENSIONS, double>* EnclaveDataRepository::getCellSizes() { return _cellSizesDevice; }
  int    EnclaveDataRepository::getNumberOfEnclaveCells() const { return _numberOfEnclaveCells; }
  int    EnclaveDataRepository::getNumberOfBoundaryFaces() const { return _numberOfBoundaryFaces; }
  bool   EnclaveDataRepository::isReady() const { return _isReady; }
  double EnclaveDataRepository::getMinVolumeH() const { return _minVolumeH; }
  double EnclaveDataRepository::getMaxVolumeH() const { return _maxVolumeH; }
} // namespace benchmarks::exahype2::kernelbenchmarks
