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
  ::std::vector<::sycl::event> copyEvent(5);
  cellCentre     = ::sycl::malloc_device<tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  cellSize       = ::sycl::malloc_device<tarch::la::Vector<Dimensions, double>>(numberOfCells, queue);
  QIn            = ::sycl::malloc_device<double>(numberOfCells * inEnumerator.size(), queue);
  QOut           = ::sycl::malloc_device<double>(numberOfCells * outEnumerator.size(), queue);
  t              = ::sycl::malloc_device<double>(numberOfCells, queue);
  dt             = ::sycl::malloc_device<double>(numberOfCells, queue);
  maxEigenvalue  = ::sycl::malloc_device<double>(numberOfCells, queue);
  double* tmpQIn = new double[numberOfCells * inEnumerator.size()];
  for (int i = 0; i < numberOfCells; i++) {
    ::std::memcpy(&tmpQIn[i * inEnumerator.size()], hostObject.QIn[i], inEnumerator.size() * sizeof(double));
  }
  copyEvent[0] = queue.memcpy(
    cellCentre, hostObject.cellCentre, numberOfCells * sizeof(tarch::la::Vector<Dimensions, double>)
  );
  copyEvent[1] = queue.memcpy(
    cellSize, hostObject.cellSize, numberOfCells * sizeof(tarch::la::Vector<Dimensions, double>)
  );
  copyEvent[2] = queue.memcpy(QIn, tmpQIn, numberOfCells * inEnumerator.size() * sizeof(double));
  copyEvent[3] = queue.memcpy(t, hostObject.t, numberOfCells * sizeof(double));
  copyEvent[4] = queue.memcpy(dt, hostObject.dt, numberOfCells * sizeof(double));
  ::sycl::event::wait(copyEvent);
  delete[] tmpQIn;
}

void exahype2::fv::rusanov::sycl::GPUCellData::destroy(
  ::exahype2::CellData&                                     hostObject,
  const ::exahype2::enumerator::AoSLexicographicEnumerator& outEnumerator,
  bool                                                      copyEigenvaluesBack,
  ::sycl::queue&                                            queue
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

#endif
