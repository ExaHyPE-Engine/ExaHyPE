// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Configuration.h"

#include "peano4/Version.h"
#include "runner/Format.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "tarch/CPUFeatureDetection.h"
#include "tarch/StackSizeLimitDetection.h"
#include "tarch/accelerator/Device.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

#include <limits>
#include <sstream>

runner::Configuration::Configuration(PhaseContext& context) :
  _context(context) {
  const auto& parameters   = _context.parameters;
  const auto& repositories = _context.repositories;
  Table       report;

  if (not parameters.getProjectName().empty()) {
    report.add("Application", "Project", parameters.getProjectName());
  }
  if (not parameters.getExecutableName().empty()) {
    report.add("Application", "Executable", parameters.getExecutableName());
  }

  report.add("Build", "Dimensions", Format::number(DIMENSIONS));
#if defined(PEANO_WITH_MPI)
  const auto& rank = tarch::mpi::Rank::getInstance();
  report.add(
    "Build",
    "MPI",
    Format::number(rank.getNumberOfRanks()) + " Rank(s) on " + Format::number(rank.getNumberOfNodes()) + " Node(s)"
  );
#else
  report.add("Build", "MPI", "off");
#endif
#if defined(PEANO_WITH_OPENMP)
  report
    .add("Build", "OpenMP", Format::number(tarch::multicore::Core::getInstance().getNumberOfThreads()) + " Thread(s)");
#else
  report.add("Build", "OpenMP", "off");
#endif
  report.add("Build", "CPU Features", Format::number(tarch::detectCPUFeatures()));
  report.add("Build", "Stack Size", Format::number(tarch::detectStackSizeLimit()));

  std::string gpuBackend = "none";
#if defined(PEANO_WITH_GPU_BACKEND)
#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
  gpuBackend = "SYCL";
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
  gpuBackend = "HIP";
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
  gpuBackend = "CUDA";
#elif defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
  gpuBackend = "Kokkos";
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
  gpuBackend = "OMPT";
#endif
  if (gpuBackend != "none") {
    gpuBackend += ", " + Format::number(tarch::accelerator::Device::getInstance().getNumberOfDevices()) + " Device(s)";
  }
#endif
  report.add("Build", "GPU Backend", gpuBackend);

  report.add(
    "Build",
    "Diagnostics",
    std::string("Trace=") + (PEANO_TRACE ? "on" : "off") + " · Assertions=" + (PEANO_ASSERTIONS ? "on" : "off") +
      " · Debug=" + (PEANO_DEBUG ? "on" : "off")
  );
  report.add("Build", "Git Branch", peano4::version::GitBranch);
  report.add("Build", "Git Commit", peano4::version::GitCommitHash);
  if (repositories.getConfigurationInformation) {
    report.add("Build", "Generated With", repositories.getConfigurationInformation());
  }
  if (not parameters.getConfigFilePath().empty()) {
    report.add("Build", "Runtime Configuration", "JSON file \"" + parameters.getConfigFilePath() + "\"");
  }

  std::ostringstream domain;
  for (int d = 0; d < DIMENSIONS; ++d) {
    domain << (d == 0 ? "" : " x ") << "[" << _context.domainOffset[d] << ", "
           << (_context.domainOffset[d] + _context.domainSize[d]) << "]";
  }
  std::ostringstream periodic;
  const char*        axis = "xyz";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (_context.periodicBC[d]) {
      periodic << (periodic.str().empty() ? "" : ", ") << axis[d];
    }
  }
  std::ostringstream gridLevels;
  gridLevels << parameters.getGridLevels() << " .. " << parameters.getMaxGridLevels();
  std::ostringstream meshSize;
  meshSize << Format::number(parameters.getMinAdmissiblePatchSize(_context.domainSize)) << " .. "
           << Format::number(parameters.getMaxAdmissiblePatchSize(_context.domainSize));

  report.add("Domain & Grid", "Domain", domain.str());
  report.add("Domain & Grid", "Periodic Boundaries", periodic.str().empty() ? "none" : periodic.str());
  report.add("Domain & Grid", "Grid Levels (min .. max)", gridLevels.str());
  report.add("Domain & Grid", "Admissible Mesh Size (min .. max)", meshSize.str());

  const auto formatTime = [](double value) -> std::string {
    return value < std::numeric_limits<double>::max() / 10.0 ? Format::number(value) : std::string("unbounded");
  };
  report.add("Time Stepping", "Min Terminal Time", formatTime(parameters.getMinTerminalTime()));
  report.add("Time Stepping", "Max Terminal Time", formatTime(parameters.getMaxTerminalTime()));
  report.add(
    "Time Stepping",
    "Max Time Steps",
    parameters.getNumberOfTimeSteps() > 0 ? Format::number(parameters.getNumberOfTimeSteps()) : std::string("unbounded")
  );
  report.add("Time Stepping", "First Plot Time Stamp", Format::number(parameters.getFirstPlotTimeStamp()));
  report.add("Time Stepping", "Plot Snapshots", Format::number(parameters.getNumberOfSnapshots()));
  report.add("Time Stepping", "Output Path", parameters.getOutputPath());
  report.add("Time Stepping", "Time Step Log Interval", Format::number(parameters.getTimeStepLogInterval()) + " s");

  report.add("Load Balancing", "Quality", Format::number(parameters.getLoadBalancingQuality()));
  report.add("Load Balancing", "Min Tree Size", Format::number(parameters.getMinSizeOfTree()));
  report.add("Load Balancing", "Max Trees per Rank", Format::number(parameters.getMaxNumberOfTreesPerRank()));
  report.add("Load Balancing", "Target Device", Format::number(parameters.getTargetDevice()));

  report.add("MPI", "Timeout Warning", Format::number(parameters.getMPITimeOutWarning()) + " s");
  report.add("MPI", "Deadlock Timeout", Format::number(parameters.getMPIDeadlockTimeOut()) + " s");
  report.add("MPI", "Deadlock Detection", parameters.isMPIDeadlockDetectionEnabled() ? "on" : "off");

  if (repositories.getConfigurationSummary) {
    for (const auto& [section, entries] : repositories.getConfigurationSummary()) {
      for (const auto& [label, value] : entries) {
        report.add(section, label, value);
      }
    }
  }

  _banners.push_back("Peano 4 (C) www.peano-framework.org");
  if (repositories.getCopyrightBanner) {
    _banners.push_back(repositories.getCopyrightBanner());
  }
  _body = report.render();
}

std::vector<std::string> runner::Configuration::widthSamples() const {
  std::vector<std::string> samples = _banners;
  samples.insert(samples.end(), _body.begin(), _body.end());
  return samples;
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
