// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  template <typename InType, typename OutType, int NumFaceTypes = 1>
  struct CellData;
} // namespace exahype2

/**
 * Holds a set of cells with the data a kernel requires to process the stored
 * patches or polynomial shape functions. The constructor allocates the
 * internal tables, and the destructor frees them.
 */
template <typename InType, typename OutType, int NumFaceTypes>
struct exahype2::CellData final {
  InType** QIn{nullptr};

  InType* (*faces)[NumFaceTypes][peano4::FacesPerCell]{nullptr};

  tarch::la::Vector<DIMENSIONS, double>* cellCentre{nullptr};
  tarch::la::Vector<DIMENSIONS, double>* cellSize{nullptr};

  OutType** QOut{nullptr};
  OutType*  maxEigenvalue{nullptr};

  double t{0.0};
  double dt{0.0};

  const int                   numberOfCells;
  const tarch::MemoryLocation memoryLocation;
  const int                   targetDevice;

  CellData(
    InType*                                      QIn_,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre_,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize_,
    double                                       t_,
    double                                       dt_,
    OutType*                                     QOut_,
    tarch::MemoryLocation                        memoryLocation_ = tarch::MemoryLocation::Heap,
    int                                          targetDevice_   = tarch::accelerator::Device::DefaultDevice
  );

  CellData(
    InType*                                      QIn_,
    InType* const                                faces_[NumFaceTypes][peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre_,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize_,
    double                                       t_,
    double                                       dt_,
    OutType*                                     QOut_,
    tarch::MemoryLocation                        memoryLocation_ = tarch::MemoryLocation::Heap,
    int                                          targetDevice_   = tarch::accelerator::Device::DefaultDevice
  );

  CellData(
    int                   numberOfCells_,
    tarch::MemoryLocation memoryLocation_ = tarch::MemoryLocation::Heap,
    int                   targetDevice_   = tarch::accelerator::Device::DefaultDevice
  );

  CellData(const CellData& copy)            = delete;
  CellData(CellData&& move)                 = delete;
  CellData& operator=(const CellData& copy) = delete;
  CellData& operator=(CellData&& move)      = delete;

  ~CellData();

  [[nodiscard]] std::string toString() const;
};

#include "exahype2/CellData.cpph"
