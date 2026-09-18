// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Configuration.h"

#include "peano4/Version.h"
#include "peano4/sfc/ActiveCurve.h"
#include "runner/Format.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "tarch/CPUFeatureDetection.h"
#include "tarch/StackSizeLimitDetection.h"
#include "tarch/accelerator/Device.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

namespace {
  /**
   * Names the point-to-point layer and the byte transports the run selected. An unset
   * environment variable reports the library's own default.
   */
  std::string mpiTransport() {
    // The report runs once at start-up before the tasking runtime creates any thread.
    const auto* const pointToPointLayer = std::getenv("OMPI_MCA_pml"); // NOLINT(concurrency-mt-unsafe)
    const auto* const byteTransport     = std::getenv("OMPI_MCA_btl"); // NOLINT(concurrency-mt-unsafe)
    if (pointToPointLayer == nullptr and byteTransport == nullptr) {
      return "library default";
    }
    std::string description = pointToPointLayer != nullptr ? pointToPointLayer : "library default";
    if (byteTransport != nullptr) {
      description += std::string(" over ") + byteTransport;
    }
    return description;
  }

  /**
   * Reports the total and the free space of the memory filesystem the OpenMP runtime and the
   * byte transports of one node map their segments into.
   */
  std::string memoryFilesystemHeadroom() {
    std::error_code error;
    const auto      space = std::filesystem::space("/dev/shm", error);
    if (error) {
      return "unavailable";
    }
    return runner::Format::memory(space.capacity) + " total · " + runner::Format::memory(space.available) + " free";
  }

  /** Names the GPU backend the build selected and, when there is one, the devices it found. */
  std::string gpuBackendLabel() {
#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
    const std::string backend = "SYCL";
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
    const std::string backend = "HIP";
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
    const std::string backend = "CUDA";
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
    const std::string backend = "Kokkos";
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
    const std::string backend = "OMPT";
#else
    const std::string backend = "none";
#endif
#if defined(PEANO_WITH_GPU_BACKEND)
    const auto numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
    return backend + ", " + runner::Format::number(numberOfDevices) + (numberOfDevices == 1 ? " Device" : " Devices");
#else
    return backend;
#endif
  }
} // namespace

runner::Configuration::Configuration(runner::PhaseContext& context) :
  _context(context) {
  const auto&   parameters = _context.parameters;
  const auto&   callbacks  = _context.repositories;
  runner::Table report;

  if (not parameters.getProjectName().empty()) {
    report.add("Application", "Project", parameters.getProjectName());
  }
  if (not parameters.getExecutableName().empty()) {
    report.add("Application", "Executable", parameters.getExecutableName());
  }

  report.add("Build", "Dimensions", runner::Format::number(DIMENSIONS));
  report.add(
    "Build",
    "Space-Filling Curve",
    std::string(peano4::sfc::ActiveCurve::Name) + " ("
      + runner::Format::number(peano4::sfc::ActiveCurve::SubdivisionFactor) + " Children per Axis)"
  );
  const auto& rank          = tarch::mpi::Rank::getInstance();
  const auto  numberOfRanks = rank.getNumberOfRanks();
  const auto  numberOfNodes = rank.getNumberOfNodes();
  report.add(
    "Build",
    "MPI",
    runner::Format::number(numberOfRanks) + (numberOfRanks == 1 ? " Rank on " : " Ranks on ")
      + runner::Format::number(numberOfNodes) + (numberOfNodes == 1 ? " Node" : " Nodes")
  );
  const auto numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  report
    .add("Build", "OpenMP", runner::Format::number(numberOfThreads) + (numberOfThreads == 1 ? " Thread" : " Threads"));
  report.add("Build", "CPU Features", runner::Format::number(tarch::detectCPUFeatures()));
  report.add("Build", "Stack Size", runner::Format::number(tarch::detectStackSizeLimit()));

  report.add("Build", "GPU Backend", gpuBackendLabel());

  report.add(
    "Build",
    "Diagnostics",
    std::string("Assertions=") + (PEANO_ASSERTIONS ? "on" : "off") + " · Debug=" + (PEANO_DEBUG ? "on" : "off")
  );
  report.add("Build", "Git Branch", peano4::version::GitBranch);
  report.add("Build", "Git Commit", peano4::version::GitCommitHash);
  if (not parameters.getConfigFilePath().empty()) {
    report.add("Build", "Runtime Configuration", "JSON file \"" + parameters.getConfigFilePath() + "\"");
  }

  std::ostringstream domain;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    domain << (dimension == 0 ? "" : " x ") << "[" << _context.domainOffset[dimension] << ", "
           << (_context.domainOffset[dimension] + _context.domainSize[dimension]) << "]";
  }
  std::ostringstream periodic;
  constexpr char     AxisNames[]         = "xyz";
  bool               hasPeriodicBoundary = false;
  for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
    if (_context.periodicBC[dimension]) {
      periodic << (hasPeriodicBoundary ? ", " : "") << AxisNames[dimension];
      hasPeriodicBoundary = true;
    }
  }
  const auto         periodicBoundaries = periodic.str();
  std::ostringstream gridLevels;
  gridLevels << parameters.getGridLevels() << " .. " << parameters.getMaxGridLevels();
  std::ostringstream meshSize;
  meshSize << runner::Format::number(parameters.getMinAdmissiblePatchSize(_context.domainSize)) << " .. "
           << runner::Format::number(parameters.getMaxAdmissiblePatchSize(_context.domainSize));

  report.add("Domain & Grid", "Domain", domain.str());
  report.add("Domain & Grid", "Periodic Boundaries", periodicBoundaries.empty() ? "none" : periodicBoundaries);
  report.add("Domain & Grid", "Grid Levels (min .. max)", gridLevels.str());
  report.add("Domain & Grid", "Admissible Patch Size (min .. max)", meshSize.str());

  const auto formatTime = [](const double value) {
    return value < std::numeric_limits<double>::max() / 10.0 ? runner::Format::number(value) : std::string("unbounded");
  };
  report.add("Time Stepping", "Minimum Terminal Time", formatTime(parameters.getMinTerminalTime()));
  report.add("Time Stepping", "Maximum Terminal Time", formatTime(parameters.getMaxTerminalTime()));
  report.add(
    "Time Stepping",
    "Maximum Time Steps",
    parameters.getNumberOfTimeSteps() > 0 ? runner::Format::number(parameters.getNumberOfTimeSteps())
                                          : std::string("unbounded")
  );
  report.add("Time Stepping", "First Plot Time Stamp", runner::Format::number(parameters.getFirstPlotTimeStamp()));
  report.add("Time Stepping", "Plot Snapshots", runner::Format::number(parameters.getNumberOfSnapshots()));
  report.add("Time Stepping", "Output Path", parameters.getOutputPath());
  report
    .add("Time Stepping", "Time Step Log Interval", runner::Format::number(parameters.getTimeStepLogInterval()) + " s");

  report.add("Accelerator", "Target Device", runner::Format::number(parameters.getTargetDevice()));

  report.add("MPI", "Transport", mpiTransport());
  report.add("MPI", "Memory Filesystem", memoryFilesystemHeadroom());
  report
    .add("MPI", "Communication Thread", tarch::mpi::AbstractCommunicationManager::usesDedicatedThread() ? "on" : "off");
  report.add("MPI", "Timeout Warning", runner::Format::number(parameters.getMPITimeoutWarning()) + " s");
  report.add("MPI", "Deadlock Timeout", runner::Format::number(parameters.getMPIDeadlockTimeout()) + " s");
  report.add("MPI", "Request Tracking", parameters.isMPIRequestTrackingEnabled() ? "on" : "off");

  if (callbacks.getConfigurationSummary) {
    for (const auto& [section, entries] : callbacks.getConfigurationSummary()) {
      for (const auto& [label, value] : entries) {
        report.add(section, label, value);
      }
    }
  }

  _banners.emplace_back("Peano 4 (C) www.peano-framework.org");
  if (callbacks.getCopyrightBanner) {
    _banners.push_back(callbacks.getCopyrightBanner());
  }
  _body = report.render();
}

void runner::Configuration::run() {
  _context.box.rule();
  for (const auto& banner : _banners) {
    _context.box.line(banner);
  }
  _context.box.rule();
  for (const auto& bodyLine : _body) {
    _context.box.line(bodyLine);
  }
  _context.box.rule();
}
