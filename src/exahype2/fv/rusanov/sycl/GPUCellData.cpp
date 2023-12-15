// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GPUCellData.h"

#if defined(GPUOffloadingSYCL)

#include <cstring>

#include "exahype2/CellData.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "tarch/accelerator/sycl/GPUMemoryManager.h"

exahype2::fv::rusanov::sycl::GPUCopyCellData::GPUCopyCellData(
  const CellData& hostObject, const enumerator::AoSLexicographicEnumerator& inEnumerator, const enumerator::AoSLexicographicEnumerator& outEnumerator, ::sycl::queue& queue
) {
  numberOfCells           = hostObject.numberOfCells;
  cellCentre              = ::sycl::malloc_device<::tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  cellSize                = ::sycl::malloc_device<::tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  QIn                     = ::sycl::malloc_device<double*>(numberOfCells, queue);
  QOut                    = ::sycl::malloc_device<double*>(numberOfCells, queue);
  QInCopyInOneHugeBuffer  = ::sycl::malloc_device<double>(numberOfCells * inEnumerator.size(), queue);
  QOutCopyInOneHugeBuffer = ::sycl::malloc_device<double>(numberOfCells * outEnumerator.size(), queue);
  t                       = ::sycl::malloc_device<double>(numberOfCells, queue);
  dt                      = ::sycl::malloc_device<double>(numberOfCells, queue);
  maxEigenvalue           = ::sycl::malloc_device<double>(numberOfCells, queue);

  std::vector<::sycl::event> copyEvent(5);
  copyEvent[0] = queue.memcpy(cellCentre, hostObject.cellCentre, numberOfCells * sizeof(::tarch::la::Vector<Dimensions, double>));
  copyEvent[1] = queue.memcpy(cellSize, hostObject.cellSize, numberOfCells * sizeof(::tarch::la::Vector<Dimensions, double>));
  copyEvent[2] = queue.memcpy(t, hostObject.t, numberOfCells * sizeof(double));
  copyEvent[3] = queue.memcpy(dt, hostObject.dt, numberOfCells * sizeof(double));

  double* tmpQIn = new double[numberOfCells * inEnumerator.size()];
  for (int i = 0; i < numberOfCells; i++) {
    ::std::memcpy(&tmpQIn[i * inEnumerator.size()], hostObject.QIn[i], inEnumerator.size() * sizeof(double));
    queue.submit([&](::sycl::handler& h) {
      h.single_task([=, *this]() {
        QIn[i]  = &QInCopyInOneHugeBuffer[i * inEnumerator.size()];
        QOut[i] = &QOutCopyInOneHugeBuffer[i * outEnumerator.size()];
      });
    });
  }
  copyEvent[4] = queue.memcpy(QInCopyInOneHugeBuffer, tmpQIn, numberOfCells * inEnumerator.size() * sizeof(double));
  delete[] tmpQIn;

  ::sycl::event::wait(copyEvent);
}

void exahype2::fv::rusanov::sycl::GPUCopyCellData::destroy(
  CellData& hostObject, const enumerator::AoSLexicographicEnumerator& outEnumerator, bool copyEigenvaluesBack, ::sycl::queue& queue
) {
  double* tmpQOut = new double[numberOfCells * outEnumerator.size()];

  std::vector<::sycl::event> copyEvent(2);
  copyEvent[0] = queue.memcpy(tmpQOut, QOutCopyInOneHugeBuffer, numberOfCells * outEnumerator.size() * sizeof(double));
  if (copyEigenvaluesBack) {
    copyEvent[1] = queue.memcpy(hostObject.maxEigenvalue, maxEigenvalue, numberOfCells * sizeof(double));
  }
  ::sycl::event::wait(copyEvent);
  for (int i = 0; i < numberOfCells; i++) {
    ::std::memcpy(hostObject.QOut[i], &tmpQOut[i * outEnumerator.size()], outEnumerator.size() * sizeof(double));
  }

  delete[] tmpQOut;
  ::sycl::free(cellCentre, queue);
  ::sycl::free(cellSize, queue);
  ::sycl::free(QIn, queue);
  ::sycl::free(QOut, queue);
  ::sycl::free(QInCopyInOneHugeBuffer, queue);
  ::sycl::free(QOutCopyInOneHugeBuffer, queue);
  ::sycl::free(t, queue);
  ::sycl::free(dt, queue);
  ::sycl::free(maxEigenvalue, queue);
}


exahype2::fv::rusanov::sycl::GPUUSMCellData::GPUUSMCellData(
  const CellData& hostObject, const enumerator::AoSLexicographicEnumerator& inEnumerator, const enumerator::AoSLexicographicEnumerator& outEnumerator, ::sycl::queue& queue
) {
  numberOfCells = hostObject.numberOfCells;

  cellCentre    = ::sycl::malloc_shared<::tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  cellSize      = ::sycl::malloc_shared<::tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  QIn           = ::sycl::malloc_shared<double*>(numberOfCells, queue);
  QOut          = ::sycl::malloc_shared<double*>(numberOfCells, queue);
  t             = ::sycl::malloc_shared<double>(numberOfCells, queue);
  dt            = ::sycl::malloc_shared<double>(numberOfCells, queue);
  maxEigenvalue = ::sycl::malloc_shared<double>(numberOfCells, queue);

  for (int i = 0; i < numberOfCells; i++) {
    QIn[i]  = hostObject.QIn[i];
    QOut[i] = hostObject.QOut[i];
  }
  std::vector<::sycl::event> copyEvent(4);
  copyEvent[0] = queue.memcpy(cellCentre, hostObject.cellCentre, numberOfCells * sizeof(::tarch::la::Vector<Dimensions, double>));
  copyEvent[1] = queue.memcpy(cellSize, hostObject.cellSize, numberOfCells * sizeof(::tarch::la::Vector<Dimensions, double>));
  copyEvent[2] = queue.memcpy(t, hostObject.t, numberOfCells * sizeof(double));
  copyEvent[3] = queue.memcpy(dt, hostObject.dt, numberOfCells * sizeof(double));
  ::sycl::event::wait(copyEvent);
}

void exahype2::fv::rusanov::sycl::GPUUSMCellData::destroy(
  CellData& hostObject, const enumerator::AoSLexicographicEnumerator& outEnumerator, bool copyEigenvaluesBack, ::sycl::queue& queue
) {
  double*                      tmpQOut = new double[numberOfCells * outEnumerator.size()];
  ::std::vector<::sycl::event> copyEvent(2);
  copyEvent[0] = queue.memcpy(tmpQOut, QOut, numberOfCells * outEnumerator.size() * sizeof(double));
  if (copyEigenvaluesBack) {
    copyEvent[1] = queue.memcpy(hostObject.maxEigenvalue, maxEigenvalue, numberOfCells * sizeof(double));
  }
  ::sycl::event::wait(copyEvent);
  for (int i = 0; i < numberOfCells; i++) {
    ::std::memcpy(hostObject.QOut[i], &tmpQOut[i * outEnumerator.size()], outEnumerator.size() * sizeof(double));
  }

  delete[] tmpQOut;
  ::sycl::free(cellCentre, queue);
  ::sycl::free(cellSize, queue);
  ::sycl::free(QIn, queue);
  ::sycl::free(QOut, queue);
  ::sycl::free(t, queue);
  ::sycl::free(dt, queue);
  ::sycl::free(maxEigenvalue, queue);
}


exahype2::fv::rusanov::sycl::GPUManagedCellData::GPUManagedCellData(
  const CellData& hostObject, const enumerator::AoSLexicographicEnumerator& inEnumerator, const enumerator::AoSLexicographicEnumerator& outEnumerator, ::sycl::queue& queue
) {
  numberOfCells           = hostObject.numberOfCells;
  cellCentre              = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<::tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  cellSize                = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<::tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  QIn                     = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double*>(numberOfCells, queue);
  QOut                    = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double*>(numberOfCells, queue);
  QInCopyInOneHugeBuffer  = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double>(numberOfCells * inEnumerator.size(), queue);
  QOutCopyInOneHugeBuffer = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double>(numberOfCells * outEnumerator.size(), queue);
  t                       = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double>(numberOfCells, queue);
  dt                      = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double>(numberOfCells, queue);
  maxEigenvalue           = ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().allocate<double>(numberOfCells, queue);

  std::vector<::sycl::event> copyEvent(5);
  copyEvent[0] = queue.memcpy(cellCentre, hostObject.cellCentre, numberOfCells * sizeof(::tarch::la::Vector<Dimensions, double>));
  copyEvent[1] = queue.memcpy(cellSize, hostObject.cellSize, numberOfCells * sizeof(::tarch::la::Vector<Dimensions, double>));
  copyEvent[2] = queue.memcpy(t, hostObject.t, numberOfCells * sizeof(double));
  copyEvent[3] = queue.memcpy(dt, hostObject.dt, numberOfCells * sizeof(double));

  double* tmpQIn = new double[numberOfCells * inEnumerator.size()];
  for (int i = 0; i < numberOfCells; i++) {
    ::std::memcpy(&tmpQIn[i * inEnumerator.size()], hostObject.QIn[i], inEnumerator.size() * sizeof(double));
    queue.submit([&](::sycl::handler& h) {
      h.single_task([=, *this]() {
        QIn[i]  = &QInCopyInOneHugeBuffer[i * inEnumerator.size()];
        QOut[i] = &QOutCopyInOneHugeBuffer[i * outEnumerator.size()];
      });
    });
  }
  copyEvent[4] = queue.memcpy(QInCopyInOneHugeBuffer, tmpQIn, numberOfCells * inEnumerator.size() * sizeof(double));
  delete[] tmpQIn;

  ::sycl::event::wait(copyEvent);
}

void exahype2::fv::rusanov::sycl::GPUManagedCellData::destroy(
  CellData& hostObject, const enumerator::AoSLexicographicEnumerator& outEnumerator, bool copyEigenvaluesBack, ::sycl::queue& queue
) {
  double*                      tmpQOut = new double[numberOfCells * outEnumerator.size()];
  ::std::vector<::sycl::event> copyEvent(2);
  copyEvent[0] = queue.memcpy(tmpQOut, QOutCopyInOneHugeBuffer, numberOfCells * outEnumerator.size() * sizeof(double));
  if (copyEigenvaluesBack) {
    copyEvent[1] = queue.memcpy(hostObject.maxEigenvalue, maxEigenvalue, numberOfCells * sizeof(double));
  }
  ::sycl::event::wait(copyEvent);
  for (int i = 0; i < numberOfCells; i++) {
    ::std::memcpy(hostObject.QOut[i], &tmpQOut[i * outEnumerator.size()], outEnumerator.size() * sizeof(double));
  }

  delete[] tmpQOut;
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(cellCentre, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(cellSize, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(QIn, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(QOut, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(QInCopyInOneHugeBuffer, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(QOutCopyInOneHugeBuffer, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(t, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(dt, queue);
  ::tarch::accelerator::sycl::GPUMemoryManager::getInstance().free(maxEigenvalue, queue);
}

#endif // GPUOffloadingSYCL
