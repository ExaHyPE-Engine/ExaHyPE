#include "GPUCellData.h"
#if defined(GPUOffloadingSYCL)

#include <cstring>

#include "exahype2/CellData.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"

exahype2::fv::rusanov::sycl::GPUCellData::GPUCellData(
  const ::exahype2::CellData&                               hostObject,
  const ::exahype2::enumerator::AoSLexicographicEnumerator& inEnumerator,
  const ::exahype2::enumerator::AoSLexicographicEnumerator& outEnumerator,
  ::sycl::queue&                                            queue
) {
  numberOfCells = hostObject.numberOfCells;
  cellCentre    = ::sycl::malloc_shared<tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  cellSize      = ::sycl::malloc_shared<tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  QIn           = ::sycl::malloc_shared<double*>(numberOfCells, queue);
  QOut          = ::sycl::malloc_shared<double*>(numberOfCells, queue);
  t             = ::sycl::malloc_shared<double>(numberOfCells, queue);
  dt            = ::sycl::malloc_shared<double>(numberOfCells, queue);
  maxEigenvalue = ::sycl::malloc_shared<double>(numberOfCells, queue);

  for (int i = 0; i < numberOfCells; i++) {
    cellCentre[i] = hostObject.cellCentre[i];
    cellSize[i]   = hostObject.cellSize[i];
    t[i]          = hostObject.t[i];
    dt[i]         = hostObject.dt[i];

    QIn[i]  = ::sycl::malloc_shared<double>(inEnumerator.size(), queue);
    QOut[i] = ::sycl::malloc_shared<double>(outEnumerator.size(), queue);

    std::memcpy(QIn[i], hostObject.QIn[i], inEnumerator.size() * sizeof(double));
  }
}

void exahype2::fv::rusanov::sycl::GPUCellData::destroy(
  ::exahype2::CellData&                                   hostObject,
  const exahype2::enumerator::AoSLexicographicEnumerator& outEnumerator,
  bool                                                    copyEigenvaluesBack,
  ::sycl::queue&                                          queue
) {
  for (int i = 0; i < numberOfCells; i++) {
    std::memcpy(hostObject.QOut[i], QOut[i], outEnumerator.size() * sizeof(double));

    ::sycl::free(QIn[i], queue);
    ::sycl::free(QOut[i], queue);
  }

  if (copyEigenvaluesBack) {
    std::memcpy(hostObject.maxEigenvalue, maxEigenvalue, numberOfCells * sizeof(double));
  }

  ::sycl::free(cellCentre, queue);
  ::sycl::free(cellSize, queue);
  ::sycl::free(QIn, queue);
  ::sycl::free(QOut, queue);
  ::sycl::free(t, queue);
  ::sycl::free(dt, queue);
  ::sycl::free(maxEigenvalue, queue);
}

#endif
