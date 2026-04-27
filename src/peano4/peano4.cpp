// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4.h"

#include "peano4/Version.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/CPUFeatureDetection.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/StackSizeLimitDetection.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/accelerator/MemoryManager.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/tarch.h"

#include <fstream>
#include <iomanip>
#include <sstream>

/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef COMPILER_HAS_UTS_NAME
#include <sys/utsname.h>
#endif

namespace {
  tarch::logging::Log _log("peano4");
  constexpr int       StartupHeaderLabelWidth = 20;

  template <typename T>
  void logStartupHeaderEntry(const std::string& label, const T& value) {
    std::ostringstream message;
    message << std::left << std::setw(StartupHeaderLabelWidth) << (label + ":") << value;
    logInfo("logStartupHeader()", message.str());
  }

  void logStartupHeader() {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      logInfo("logStartupHeader()", "Peano 4 (C) www.peano-framework.org");
      logStartupHeaderEntry("Dimensions", DIMENSIONS);

      std::string mpiInfo = "Off";
#if defined(WITH_MPI)
      mpiInfo = std::to_string(tarch::mpi::Rank::getInstance().getNumberOfRanks()) + " Rank(s)";
#endif
      logStartupHeaderEntry("MPI", mpiInfo);

      std::string openmpInfo = "Off";
#if defined(WITH_OPENMP)
      openmpInfo = std::to_string(tarch::multicore::Core::getInstance().getNumberOfThreads()) + " Thread(s)";
#endif
      logStartupHeaderEntry("OpenMP", openmpInfo);

      logStartupHeaderEntry("CPU Features", tarch::detectCPUFeatures());
      logStartupHeaderEntry("Stack Size", tarch::detectStackSizeLimit());

      std::string gpuInfo = "None";
#if defined(WITH_GPU)
#if defined(WITH_GPU_SYCL)
      gpuInfo = "SYCL";
#elif defined(WITH_GPU_HIP)
      gpuInfo = "HIP";
#elif defined(WITH_GPU_CUDA)
      gpuInfo = "CUDA";
#elif defined(WITH_GPU_KOKKOS)
      gpuInfo = "Kokkos";
#elif defined(WITH_GPU_OMPT)
      gpuInfo = "OMPT";
#endif
      if (gpuInfo != "None") {
        gpuInfo += ", ";
        gpuInfo += std::to_string(tarch::accelerator::Device::getInstance().getNumberOfDevices());
        gpuInfo += " Device(s)";
      }
#endif
      logStartupHeaderEntry("GPU Backend", gpuInfo);

      logStartupHeaderEntry("Debug Level", PEANO_DEBUG);
      logStartupHeaderEntry("Git Branch", peano4::version::GitBranch);
      logStartupHeaderEntry("Git Commit Message", peano4::version::GitCommitMessage);
      logStartupHeaderEntry("Git Commit Hash", peano4::version::GitCommitHash);
    }
  }
} // namespace

void peano4::init(
  int*                                         argc,
  char***                                      argv,
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  peano4::utils::setupLookupTableForDLinearised();
  peano4::utils::setupLookupTableForDDelinearised();

  if (tarch::mpi::Rank::getInstance().init(argc, argv)) {
    peano4::parallel::Node::initMPIDatatypes();

    clock_t timeout = 3600;
    tarch::mpi::Rank::getInstance().setTimeOutWarning(timeout / 4);
    tarch::mpi::Rank::getInstance().setDeadlockTimeOut(timeout);
  } else {
    throw std::runtime_error("Cannot initialise parallel environment");
  }

  if (tarch::multicore::Core::getInstance().init()) {
  } else {
    throw std::runtime_error("Cannot initialise shared memory environment");
  }

  if (tarch::accelerator::Device::getInstance().init()) {
  } else {
    throw std::runtime_error("Cannot initialise accelerator environment");
  }

  tarch::initNonCriticalAssertionEnvironment();
  tarch::services::ServiceRepository::getInstance().init();
  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().init();
  peano4::parallel::Node::getInstance().init();
  peano4::parallel::SpacetreeSet::getInstance().init(offset, width, periodicBC);

  logStartupHeader();
}

void peano4::shutdown() {
  peano4::parallel::Node::getInstance().shutdown();
  peano4::parallel::Node::shutdownMPIDatatypes();
  peano4::parallel::SpacetreeSet::getInstance().shutdown();
  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().shutdown();
  tarch::services::ServiceRepository::getInstance().shutdown();
  tarch::shutdownNonCriticalAssertionEnvironment();
  tarch::accelerator::MemoryManager::getInstance().clear();
  tarch::accelerator::Device::getInstance().shutdown();
  tarch::accelerator::MemoryBlocks::getInstance().checkForMemoryLeaks();
  tarch::accelerator::MemoryBlocks::getInstance().clear();
  tarch::multicore::Core::getInstance().shutdown();
  tarch::mpi::Rank::getInstance().shutdown();

  logInfo("shutdown()", "Peano terminated successfully");
}
