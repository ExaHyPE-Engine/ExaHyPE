// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "Constants.h"

// #include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
// #include "repositories/StepRepository.h"

#include "exahype2/CellData.h"
#include "exahype2/dg/DGUtils.h"
#include "Utils.h"

#include "kernels/AderSolver/BufferSizes.h"
#include "kernels/AderSolver/DGMatrices.h"
#include "kernels/AderSolver/FaceIntegral.h"
#include "kernels/AderSolver/FusedSpaceTimePredictorVolumeIntegral.h"
#include "kernels/AderSolver/MaxScaledEigenvalue.h"
#include "kernels/AderSolver/Quadrature.h"
#include "kernels/AderSolver/RiemannSolver.h"
#include "peano4/peano4.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "Variants.h"

#include <cstring>
#include <fstream>

using namespace benchmarks::exahype2::kernelbenchmarks;

tarch::logging::Log _log("::");

constexpr double timeStamp    = 0.0;
constexpr double timeStepSize = 1e-6;
const tarch::la::Vector<DIMENSIONS, double>
  cellCenter = benchmarks::exahype2::kernelbenchmarks::DomainOffset
               + 0.5 * benchmarks::exahype2::kernelbenchmarks::DomainSize;
const tarch::la::Vector<DIMENSIONS, double> cellSize = benchmarks::exahype2::kernelbenchmarks::DomainSize / 81.0;
// constexpr int    HaloSize     = 1;

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

int main(int argc, char** argv) {
  peano4::init(&argc, &argv, benchmarks::exahype2::kernelbenchmarks::DomainOffset, benchmarks::exahype2::kernelbenchmarks::DomainSize);
  repositories::initLogFilters();

  logInfo("main()", "number of compute threads: " << tarch::multicore::Core::getInstance().getNumberOfThreads());
  logInfo("main()", "number of threads launching compute kernels: " << NumberOfLaunchingThreads);
  logInfo("main()", "number of unknowns: " << AderSolver::NumberOfUnknowns);
  logInfo("main()", "number of auxiliary variables: " << AderSolver::NumberOfAuxiliaryVariables);
  logInfo("main()", "number of finite volumes per axis per cell: " << AderSolver::Order);
  logInfo("main()", "number of samples per measurement: " << NumberOfSamples);
  logInfo("main()", "performing accuracy checks with precision: " << Accuracy);
#if defined(WITH_GPU_SYCL)
  logInfo("main()", "set SYCL_DEVICE_FILTER=gpu or ONEAPI_DEVICE_SELECTOR=cuda:0 when using SYCL on the device");
  logInfo("main()", "set SYCL_PI_TRACE=2 in case of runtime errors");
#endif

  std::ostringstream snapshotFileName;
  snapshotFileName << "ader-benchmarks-prec-" 
                   << typeid(SolverPrecision).name()
                   << ".csv";
  std::ofstream file( snapshotFileName.str() );
  file << "Variant,omp threads,cells,total compute time,total time (including launch),avg compute time,avg time,max,min,std deviation" << std::endl;

  for (int i = 0; i < NumberOfCellsToStudy.size(); i++) {
    logInfo("main()", "number of cells: " << NumberOfCellsToStudy[i]);
    variant1::runBenchmarks(NumberOfCellsToStudy[i], timeStamp, timeStepSize, cellCenter, cellSize);
    variant2::runBenchmarks(NumberOfCellsToStudy[i], timeStamp, timeStepSize, cellCenter, cellSize);
    variant3::runBenchmarks(NumberOfCellsToStudy[i], timeStamp, timeStepSize, cellCenter, cellSize);
    variant4::runBenchmarks(NumberOfCellsToStudy[i], timeStamp, timeStepSize, cellCenter, cellSize);
    variant5::runBenchmarks(NumberOfCellsToStudy[i], timeStamp, timeStepSize, cellCenter, cellSize);
    variant6::runBenchmarks(NumberOfCellsToStudy[i], timeStamp, timeStepSize, cellCenter, cellSize);
  }

  peano4::shutdown();

  // if (outcomeIsInvalid) {
  //   return EXIT_FAILURE; // Make sure the CI pipeline reports an error
  // }

  return EXIT_SUCCESS;
}
