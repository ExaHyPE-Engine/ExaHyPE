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
    _Q_oldDevice(nullptr),
    _cellCentresDevice(nullptr),
    _cellSizesDevice(nullptr),
    _FaceEstimatesDevice(nullptr),
    _FaceFluxesDevice(nullptr) {}

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

    _numberOfEnclaveCells                    = _bookmarkedCellCentres.size();
    DataRepository& hostRepo                 = DataRepository::getInstance();
    const auto      qCardinality             = hostRepo.getCellQCardinality();
    const auto      qOldCardinality          = hostRepo.getcellQOldCardinality();
    const auto      faceEstimatesCardinality = DataRepository::getInstance().getFaceEstimatesCardinality();
    const auto      faceFluxesCardinality    = DataRepository::getInstance().getFaceFluxEstimatesCardinality();

    // --- 1. Allocate non-face GPU Memory ---
    const size_t qSize              = static_cast<size_t>(_numberOfEnclaveCells) * static_cast<size_t>(qCardinality);
    const size_t qReconstructedSize = static_cast<size_t>(_numberOfEnclaveCells) * static_cast<size_t>(qOldCardinality);
    const size_t totalQSize         = qSize + qReconstructedSize;

    _combinedQBlockDevice = tarch::allocateMemory<SolverStoragePrecision>(totalQSize, tarch::MemoryLocation::Device);
    _QDevice              = _combinedQBlockDevice;
    _Q_oldDevice          = _combinedQBlockDevice + qSize;

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
        _Q_oldDevice + i * qOldCardinality,
        hostRepo.getcellQOld(cellCentre, cellSize),
        qOldCardinality,
        tarch::MemoryLocation::Device
      );
    }

    // --- 3. Build Unique Face Pool on Host ---
    // This map will store a unique key for each face and map it to its dense index (0, 1, 2...)
    // in our new global pool.
    std::vector<SolverCorrectorPrecision> hostGlobalFaceEstimatesPool;
    std::vector<SolverCorrectorPrecision> hostGlobalFaceFluxesPool;
    std::vector<uint64_t>                 indicesOfUniqueFacesIntoGlobalFacePool;
    std::vector<uint64_t>                 indicesOfBoundaryFacesIntoGlobalFacePool;
    std::vector<uint8_t>                  hostBoundaryFaceNumbers;
    std::vector<uint64_t>                 hostBoundaryCellIndices;
    std::vector<uint8_t>                  hostFaceNumbers;
    std::vector<uint64_t>                 hostFaceCellIndices;
    hostGlobalFaceEstimatesPool.reserve(_numberOfEnclaveCells * 2 * faceEstimatesCardinality); // Pre-allocate rough
                                                                                               // estimate
    hostGlobalFaceFluxesPool.reserve(_numberOfEnclaveCells * 2 * faceFluxesCardinality); // Pre-allocate rough estimate

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

          auto* srcFaceEstimatesData = hostRepo.getFaceQEstimates(
            std::get<0>(faceKey),
            std::get<1>(faceKey),
            std::get<2>(faceKey)
          );
          hostGlobalFaceEstimatesPool.insert(
            hostGlobalFaceEstimatesPool.end(),
            srcFaceEstimatesData,
            srcFaceEstimatesData + faceEstimatesCardinality
          );

          auto* srcFaceFluxesData = hostRepo.getFaceQFluxEstimates(
            std::get<0>(faceKey),
            std::get<1>(faceKey),
            std::get<2>(faceKey)
          );
          hostGlobalFaceFluxesPool
            .insert(hostGlobalFaceFluxesPool.end(), srcFaceFluxesData, srcFaceFluxesData + faceFluxesCardinality);

          indicesOfUniqueFacesIntoGlobalFacePool.push_back(globalFaceIndex);
          hostFaceNumbers.push_back(f);
          hostFaceCellIndices.push_back(i);
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
    // std::cout << "host global face pool size: " << hostGlobalFacePool.size() << std::endl;
    // _combinedFaceBlockDevice =
    // tarch::allocateMemory<SolverCorrectorPrecision>(hostGlobalFaceEstimatesPool.size()+hostGlobalFaceFluxesPool.size(),
    // tarch::MemoryLocation::Device);
    _FaceEstimatesDevice = tarch::allocateMemory<SolverCorrectorPrecision>(
      hostGlobalFaceEstimatesPool.size(),
      tarch::MemoryLocation::Device
    );
    _FaceFluxesDevice = tarch::allocateMemory<SolverCorrectorPrecision>(
      hostGlobalFaceFluxesPool.size(),
      tarch::MemoryLocation::Device
    );
    _indicesOfUniqueFacesDevice = tarch::allocateMemory<uint64_t>(
      indicesOfUniqueFacesIntoGlobalFacePool.size(),
      tarch::MemoryLocation::Device
    );
    _indicesOfBoundaryFacesDevice = tarch::allocateMemory<uint64_t>(
      indicesOfBoundaryFacesIntoGlobalFacePool.size(),
      tarch::MemoryLocation::Device
    );
    // _FaceEstimatesDevice = _combinedFaceBlockDevice;
    // _FaceFluxesDevice = _combinedFaceBlockDevice + hostGlobalFaceEstimatesPool.size();

    _boundaryFaceNumbersDevice = tarch::allocateMemory<uint8_t>(
      hostBoundaryFaceNumbers.size(),
      tarch::MemoryLocation::Device
    );
    _boundaryCellIndicesDevice = tarch::allocateMemory<uint64_t>(
      hostBoundaryCellIndices.size(),
      tarch::MemoryLocation::Device
    );

    _faceNumbersDevice     = tarch::allocateMemory<uint8_t>(hostFaceNumbers.size(), tarch::MemoryLocation::Device);
    _faceCellIndicesDevice = tarch::allocateMemory<uint64_t>(hostFaceCellIndices.size(), tarch::MemoryLocation::Device);

    // tarch::copyTo(
    //   _combinedFaceBlockDevice,
    //   hostGlobalFaceEstimatesPool.data(),
    //   hostGlobalFaceEstimatesPool.size(),
    //   tarch::MemoryLocation::Device
    // );
    // tarch::copyTo(
    //   _combinedFaceBlockDevice+hostGlobalFaceEstimatesPool.size(),
    //   hostGlobalFaceFluxesPool.data(),
    //   hostGlobalFaceFluxesPool.size(),
    //   tarch::MemoryLocation::Device
    // );
    tarch::copyTo(
      _FaceEstimatesDevice,
      hostGlobalFaceEstimatesPool.data(),
      hostGlobalFaceEstimatesPool.size(),
      tarch::MemoryLocation::Device
    );
    tarch::copyTo(
      _FaceFluxesDevice,
      hostGlobalFaceFluxesPool.data(),
      hostGlobalFaceFluxesPool.size(),
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

    tarch::copyTo(
      _indicesOfUniqueFacesDevice,
      indicesOfUniqueFacesIntoGlobalFacePool.data(),
      indicesOfUniqueFacesIntoGlobalFacePool.size(),
      tarch::MemoryLocation::Device
    );
    tarch::copyTo(_faceNumbersDevice, hostFaceNumbers.data(), hostFaceNumbers.size(), tarch::MemoryLocation::Device);
    tarch::copyTo(
      _faceCellIndicesDevice,
      hostFaceCellIndices.data(),
      hostFaceCellIndices.size(),
      tarch::MemoryLocation::Device
    );
    _numberOfFaces = indicesOfUniqueFacesIntoGlobalFacePool.size();

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
    // tarch::freeMemory(_combinedFaceBlockDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_FaceEstimatesDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_FaceFluxesDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_indicesOfUniqueFacesDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_faceNumbersDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_faceCellIndicesDevice, tarch::MemoryLocation::Device);

    // Clear boundary data
    tarch::freeMemory(_boundaryFaceNumbersDevice, tarch::MemoryLocation::Device);
    tarch::freeMemory(_boundaryCellIndicesDevice, tarch::MemoryLocation::Device);

    _QDevice           = nullptr;
    _Q_oldDevice       = nullptr;
    _cellCentresDevice = nullptr;
    _cellSizesDevice   = nullptr;

    _FaceEstimatesDevice = nullptr;
    _FaceFluxesDevice    = nullptr;

    _indicesOfBoundaryFacesDevice = nullptr;
    _indicesOfFacesDevice         = nullptr;
    _boundaryFaceNumbersDevice    = nullptr;
    _boundaryCellIndicesDevice    = nullptr;
    _indicesOfUniqueFacesDevice   = nullptr;
    _faceNumbersDevice            = nullptr;
    _faceCellIndicesDevice        = nullptr;
    _isReady                      = false;
    _numberOfEnclaveCells         = 0;
    _numberOfFaces                = 0;
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

    DataRepository& hostRepo                 = DataRepository::getInstance();
    const auto      faceEstimatesCardinality = DataRepository::getInstance().getFaceEstimatesCardinality();
    const auto      faceFluxesCardinality    = DataRepository::getInstance().getFaceFluxEstimatesCardinality();

    for (auto face : _uniqueFaceMap) {
      // faceIndex, axis
      FaceKey  faceKey         = face.first;
      uint64_t globalFaceIndex = face.second;
      tarch::copyFrom(
        _FaceEstimatesDevice + globalFaceIndex * faceEstimatesCardinality,
        hostRepo.getFaceQEstimates(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceEstimatesCardinality,
        tarch::MemoryLocation::Device
      );
      tarch::copyFrom(
        _FaceFluxesDevice + globalFaceIndex * faceFluxesCardinality,
        hostRepo.getFaceQFluxEstimates(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceFluxesCardinality,
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

    const auto      faceEstimatesCardinality = DataRepository::getInstance().getFaceEstimatesCardinality();
    const auto      faceFluxesCardinality    = DataRepository::getInstance().getFaceFluxEstimatesCardinality();
    DataRepository& hostRepo                 = DataRepository::getInstance();
    for (auto face : _uniqueFaceMap) {
      // faceIndex, axis
      FaceKey  faceKey         = face.first;
      uint64_t globalFaceIndex = face.second;
      tarch::copyTo(
        _FaceEstimatesDevice + globalFaceIndex * faceEstimatesCardinality,
        hostRepo.getFaceQEstimates(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceEstimatesCardinality,
        tarch::MemoryLocation::Device
      );
      tarch::copyTo(
        _FaceFluxesDevice + globalFaceIndex * faceFluxesCardinality,
        hostRepo.getFaceQFluxEstimates(std::get<0>(faceKey), std::get<1>(faceKey), std::get<2>(faceKey)),
        faceFluxesCardinality,
        tarch::MemoryLocation::Device
      );
    }
  }

  // --- GPU Data Accessors ---
  SolverStoragePrecision*   EnclaveDataRepository::getQ() { return _QDevice; }
  SolverStoragePrecision*   EnclaveDataRepository::getQOld() { return _Q_oldDevice; }
  SolverCorrectorPrecision* EnclaveDataRepository::getFaceEstimatesPool() { return _FaceEstimatesDevice; }
  SolverCorrectorPrecision* EnclaveDataRepository::getFaceFluxesPool() { return _FaceFluxesDevice; }
  uint64_t*                 EnclaveDataRepository::getFaceIndices() { return _indicesOfFacesDevice; }
  uint64_t*                 EnclaveDataRepository::getUniqueFaceIndices() { return _indicesOfUniqueFacesDevice; }
  uint64_t*                 EnclaveDataRepository::getBoundaryFaceIndices() { return _indicesOfBoundaryFacesDevice; }
  uint8_t*                  EnclaveDataRepository::getBoundaryFaceNumbers() { return _boundaryFaceNumbersDevice; }
  uint64_t*                 EnclaveDataRepository::getBoundaryCellIndices() { return _boundaryCellIndicesDevice; }
  uint8_t*                  EnclaveDataRepository::getFaceNumbers() { return _faceNumbersDevice; }
  uint64_t*                 EnclaveDataRepository::getFaceCellIndices() { return _faceCellIndicesDevice; }
  tarch::la::Vector<DIMENSIONS, double>* EnclaveDataRepository::getCellCentres() { return _cellCentresDevice; }
  tarch::la::Vector<DIMENSIONS, double>* EnclaveDataRepository::getCellSizes() { return _cellSizesDevice; }
  int    EnclaveDataRepository::getNumberOfEnclaveCells() const { return _numberOfEnclaveCells; }
  int    EnclaveDataRepository::getNumberOfFaces() const { return _numberOfFaces; }
  int    EnclaveDataRepository::getNumberOfBoundaryFaces() const { return _numberOfBoundaryFaces; }
  bool   EnclaveDataRepository::isReady() const { return _isReady; }
  double EnclaveDataRepository::getMinVolumeH() const { return _minVolumeH; }
  double EnclaveDataRepository::getMaxVolumeH() const { return _maxVolumeH; }
} // namespace benchmarks::exahype2::kernelbenchmarks
