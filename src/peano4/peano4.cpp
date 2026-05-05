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
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/tarch.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef COMPILER_HAS_UTS_NAME
#include <sys/utsname.h>
#endif

namespace {
  constexpr int StartupHeaderLabelWidth = 20;

  template <typename T>
  void appendStartupHeaderEntry(std::vector<std::string>& lines, const std::string& label, const T& value) {
    std::ostringstream message;
    message << std::left << std::setw(StartupHeaderLabelWidth) << (label + ":") << value;
    lines.push_back(message.str());
  }

  std::vector<std::string> createStartupHeaderLines() {
    std::vector<std::string> result;
    result.push_back("Peano 4 (C) www.peano-framework.org");
    appendStartupHeaderEntry(result, "Dimensions", DIMENSIONS);

    std::string mpiInfo = "Off";
#if defined(WITH_MPI)
    mpiInfo = std::to_string(tarch::mpi::Rank::getInstance().getNumberOfRanks()) + " Rank(s)";
#endif
    appendStartupHeaderEntry(result, "MPI", mpiInfo);

    std::string openmpInfo = "Off";
#if defined(WITH_OPENMP)
    openmpInfo = std::to_string(tarch::multicore::Core::getInstance().getNumberOfThreads()) + " Thread(s)";
#endif
    appendStartupHeaderEntry(result, "OpenMP", openmpInfo);

    appendStartupHeaderEntry(result, "CPU Features", tarch::detectCPUFeatures());
    appendStartupHeaderEntry(result, "Stack Size", tarch::detectStackSizeLimit());

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
    appendStartupHeaderEntry(result, "GPU Backend", gpuInfo);

    appendStartupHeaderEntry(
      result,
      "Diagnostics",
      std::string("Trace=") + (PEANO_TRACE ? "On" : "Off") + ", Assertions=" + (PEANO_ASSERTIONS ? "On" : "Off")
        + ", Debug=" + (PEANO_DEBUG ? "On" : "Off")
    );
    appendStartupHeaderEntry(result, "Git Branch", peano4::version::GitBranch);
    appendStartupHeaderEntry(result, "Git Commit Message", peano4::version::GitCommitMessage);
    appendStartupHeaderEntry(result, "Git Commit Hash", peano4::version::GitCommitHash);

    return result;
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

  for (const std::string& line : createStartupHeaderLines()) {
    logInfo(line);
  }
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
  logInfo("Peano terminated successfully");
  tarch::instrumentation::Log::close();
  tarch::mpi::Rank::getInstance().shutdown();
}
