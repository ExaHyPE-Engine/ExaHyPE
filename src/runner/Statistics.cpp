// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "peano4/Version.h"
#include "runner/LoadBalancer.h"
#include "runner/RuntimeParameters.h"
#include "tarch/CPUFeatureDetection.h"
#include "tarch/MemoryUsage.h"
#include "tarch/StackSizeLimitDetection.h"
#include "tarch/accelerator/Device.h"
#include "tarch/instrumentation/Info.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {
  // Fixed inner box width. Every box is printed at this width on every run.
  constexpr std::size_t BoxWidth = 100;

  // Columns the frame adds around the inner content: "| " on the left and " |" on the
  // right. frameBoxLines draws exactly this, so a box line is BoxFrameColumns + width.
  constexpr std::size_t BoxFrameColumns = 4;

  // Minimum gap kept between the box title and the right-aligned progress indicator.
  constexpr std::size_t HeaderProgressGap = 3;

  // Placeholder so a row keeps its slot until its value is available.
  constexpr auto NotYet = "—";

  // Display columns (UTF-8 code points) so multi-byte glyphs do not push the border out.
  std::size_t displayWidth(const std::string& text) {
    std::size_t columns = 0;
    for (const unsigned char character : text) {
      if ((character & 0xC0) != 0x80) {
        ++columns;
      }
    }
    return columns;
  }

  // Truncate to at most maxColumns display columns, without splitting a UTF-8 codepoint.
  std::string truncateToDisplayWidth(const std::string& text, std::size_t maxColumns) {
    std::size_t columns = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
      if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) {
        if (columns == maxColumns) {
          return text.substr(0, i);
        }
        ++columns;
      }
    }
    return text;
  }

  // Frame the header/body into the full set of box lines (rules included) at the fixed
  // box width. A header-only box (no body) closes right after the header.
  std::vector<std::string>
  frameBoxLines(const std::vector<std::string>& headerLines, const std::vector<std::string>& bodyLines, char rule) {
    // "+" corners plus a rule spanning the inner width (content + the two border spaces),
    // so the rule line is the same BoxFrameColumns + BoxWidth total as a framed row.
    const std::string ruleLine = "+" + std::string(BoxWidth + BoxFrameColumns - 2, rule) + "+";
    const auto        frameRow = [](const std::string& content) {
      // Clip content to the box width so an over-long line cannot overflow the frame.
      const std::string fitted  = truncateToDisplayWidth(content, BoxWidth);
      const std::size_t columns = displayWidth(fitted);
      const std::string padding = columns < BoxWidth ? std::string(BoxWidth - columns, ' ') : std::string();
      return "| " + fitted + padding + " |";
    };

    std::vector<std::string> lines;
    lines.push_back(ruleLine);
    for (const auto& line : headerLines) {
      lines.push_back(frameRow(line));
    }
    lines.push_back(ruleLine);
    if (not bodyLines.empty()) {
      for (const auto& line : bodyLines) {
        lines.push_back(frameRow(line));
      }
      lines.push_back(ruleLine);
    }
    return lines;
  }

  std::string memory(unsigned long long bytes) {
    std::ostringstream result;
    const auto         format = tarch::chooseMemoryUsageFormat(bytes);
    result << std::fixed << std::setprecision(2) << tarch::convertMemorySize(bytes, format) << " "
           << tarch::toString(format);
    return result.str();
  }

  std::string percentage(double value) {
    std::ostringstream result;
    result << std::fixed << std::setprecision(2) << value << " %";
    return result.str();
  }

  std::string rate(double value, const char* unit) {
    std::ostringstream result;
    result << std::scientific << std::setprecision(4) << value << " " << unit;
    return result.str();
  }

  template <typename T>
  std::string number(const T& value) {
    std::ostringstream result;
    result << value;
    return result.str();
  }
} // namespace

std::string runner::formatWallClock(double seconds) {
  std::ostringstream result;
  result << std::fixed << std::setprecision(2) << seconds << " s";
  return result.str();
}

std::string runner::displayLabel(const std::string& stepName) {
  if (stepName == "CreateGrid") {
    return "Grid Construction";
  }
  if (stepName == "LoadBalancing") {
    return "Load Balancing";
  }
  if (stepName == "InitGrid") {
    return "Grid Initialisation";
  }
  if (stepName == "PlotSolution") {
    return "Plot";
  }
  if (stepName == "TimeStep") {
    return "Time Step";
  }
  return stepName;
}

std::string runner::formatProgressBar(double fraction, int width) {
  if (fraction < 0.0) {
    fraction = 0.0;
  }
  if (fraction > 1.0) {
    fraction = 1.0;
  }
  const int filled = static_cast<int>(fraction * width + 0.5);
  return "[" + std::string(static_cast<std::size_t>(filled), '#') +
         std::string(static_cast<std::size_t>(width - filled), '-') + "]";
}

// ---------------------------------------------------------------------------
// StatisticsReport
// ---------------------------------------------------------------------------
runner::StatisticsReport::Section& runner::StatisticsReport::sectionFor(const std::string& title) {
  for (auto& section : _sections) {
    if (section.title == title) {
      return section;
    }
  }
  _sections.push_back(Section{title, {}});
  return _sections.back();
}

void runner::StatisticsReport::add(
  const std::string& sectionLabel,
  const std::string& label,
  const std::string& value
) {
  sectionFor(sectionLabel).entries.emplace_back(label, value);
}

std::vector<std::string> runner::StatisticsReport::renderContent() const {
  std::vector<std::string> lines;

  std::size_t labelWidth = 0;
  for (const auto& section : _sections) {
    for (const auto& entry : section.entries) {
      labelWidth = std::max(labelWidth, entry.first.size());
    }
  }

  for (const auto& section : _sections) {
    if (section.entries.empty()) {
      continue;
    }
    lines.push_back("  " + section.title);
    for (const auto& [label, value] : section.entries) {
      std::ostringstream line;
      line << "    " << std::left << std::setw(static_cast<int>(labelWidth)) << label << "  " << value;
      lines.push_back(line.str());
    }
  }
  return lines;
}

// ---------------------------------------------------------------------------
// StatisticsLog
// ---------------------------------------------------------------------------
runner::StatisticsLog::StatisticsLog(
  const RepositoryCallbacks&                   repositories,
  MainMeasurements&                            measurements,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainSize,
  const std::bitset<DIMENSIONS>&               periodicBC
) :
  _repositories(repositories),
  _measurements(measurements),
  _domainOffset(domainOffset),
  _domainSize(domainSize),
  _periodicBC(periodicBC) {}

void runner::StatisticsLog::registerDefaultProviders() {
  // Grid: leaf/vertex counts and the level range.
  _baseProviders.push_back([](StatisticsReport& report) {
    const auto& statistics = loadBalancer().getStatistics();

    const auto leaves  = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
    const auto local   = static_cast<unsigned long>(statistics.getGlobalNumberOfVertices());
    const auto hanging = static_cast<unsigned long>(statistics.getGlobalNumberOfHangingVertices());

    std::ostringstream vertices;
    vertices << (local + hanging) << "  (Local " << local << ", Hanging " << hanging << ")";
    std::ostringstream levels;
    levels << statistics.getGlobalMinDepth() << " .. " << statistics.getGlobalMaxDepth();

    report.add("Grid", "Number of Leaves", number(leaves));
    report.add("Grid", "Number of Vertices", vertices.str());
    report.add("Grid", "Grid Levels", levels.str());
  });

  // Load balancing: tree distribution with the global efficiency (min/max tree load) and global
  // overhead (outer/inner cells); both are global, not per-rank.
  _baseProviders.push_back([](StatisticsReport& report) {
    const auto& statistics = loadBalancer().getStatistics();

    const auto inner   = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
    const auto outer   = static_cast<unsigned long>(statistics.getGlobalNumberOfOuterUnrefinedCells());
    const auto trees   = static_cast<unsigned long>(statistics.getGlobalNumberOfTrees());
    const auto ranks   = static_cast<unsigned long>(statistics.getGlobalNumberOfRanksWithAtLeastOneTree());
    const auto minLoad = static_cast<unsigned long>(statistics.getGlobalMinTreeLoad());
    const auto maxLoad = static_cast<unsigned long>(statistics.getGlobalMaxTreeLoad());

    const auto efficiency = (maxLoad > 0) ? (static_cast<double>(minLoad) / maxLoad) * 100.0 : 100.0;
    const auto overhead   = (inner > 0) ? (static_cast<double>(outer) / inner) * 100.0 : 0.0;

    std::ostringstream treeText;
    treeText << trees << "  (on " << ranks << " Rank(s))";
    std::ostringstream efficiencyText;
    efficiencyText << percentage(efficiency) << "  (" << minLoad << " .. " << maxLoad << ")";

    report.add("Load Balancing", "Number of Trees", treeText.str());
    report.add("Load Balancing", "Efficiency", efficiencyText.str());
    report.add("Load Balancing", "Overhead", percentage(overhead));
  });

  // Memory: grid plus the project's cell/face/vertex data (each callback is optional).
  _baseProviders.push_back([this](StatisticsReport& report) {
    const auto& statistics = loadBalancer().getStatistics();

    const auto bytesFrom = [](const std::function<std::size_t()>& callback) -> unsigned long long {
      return callback ? callback() : 0;
    };
    const unsigned long long gridMemory   = static_cast<unsigned long long>(statistics.getGlobalMemoryUsageInBytes());
    const unsigned long long cellMemory   = bytesFrom(_repositories.getGlobalCellDataMemoryUsageInBytes);
    const unsigned long long faceMemory   = bytesFrom(_repositories.getGlobalFaceDataMemoryUsageInBytes);
    const unsigned long long vertexMemory = bytesFrom(_repositories.getGlobalVertexDataMemoryUsageInBytes);

    report.add("Memory", "Grid", memory(gridMemory));
    report.add("Memory", "Cell Data", memory(cellMemory));
    report.add("Memory", "Face Data", memory(faceMemory));
    report.add("Memory", "Vertex Data", memory(vertexMemory));
    report.add("Memory", "Total", memory(gridMemory + cellMemory + faceMemory + vertexMemory));
    report.add("Memory", "Process (RSS)", memory(statistics.getGlobalProcessMemoryUsageInBytes()));
  });

  // --- Solver sections: added only once time stepping has started (Solver, Performance) ---

  if (_repositories.getSolverStatistics) {
    _solverProviders.push_back([this](StatisticsReport& report) {
      for (const auto& [name, statistics] : _repositories.getSolverStatistics()) {
        report.add("Solver", name, statistics);
      }
    });
  }

  _solverProviders.push_back([this](StatisticsReport& report) {
    report.add(
      "Performance",
      "Sweep(s) per Step",
      _measurements.gridSweep.getNumberOfMeasurements() > 0
        ? number(static_cast<unsigned long>(_measurements.gridSweep.getMostRecentValue()))
        : NotYet
    );

    std::string cellThroughput = NotYet;
    std::string dofThroughput  = NotYet;
    if (_measurements.timeStep.getNumberOfMeasurements() > 0) {
      const auto   leaves = static_cast<double>(loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
      const double dt     = _measurements.timeStep.getMostRecentValue();
      if (dt > 0.0) {
        cellThroughput = rate(leaves / dt, "Cells/s");
        if (_repositories.getNumberOfDoFs) {
          dofThroughput = rate(static_cast<double>(_repositories.getNumberOfDoFs()) * leaves / dt, "DoF/s");
        }
      }
    }
    report.add("Performance", "Cell Throughput", cellThroughput);
    report.add("Performance", "DoF Throughput", dofThroughput);
  });
}

void runner::StatisticsLog::drawBox(
  const std::vector<std::string>& headerLines,
  const std::vector<std::string>& bodyLines,
  char                            rule
) {
  for (const auto& line : frameBoxLines(headerLines, bodyLines, rule)) {
    logInfo(line);
  }
}

void runner::StatisticsLog::logConfiguration() {
  const auto& parameters = RuntimeParameters::getInstance();

  StatisticsReport report;

  // --- Application ---
  if (not parameters.getProjectName().empty()) {
    report.add("Application", "Project", parameters.getProjectName());
  }
  if (not parameters.getExecutableName().empty()) {
    report.add("Application", "Executable", parameters.getExecutableName());
  }

  // --- Build & system ---
  report.add("Build", "Dimensions", number(DIMENSIONS));
#if defined(PEANO_WITH_MPI)
  report.add("Build", "MPI", number(tarch::mpi::Rank::getInstance().getNumberOfRanks()) + " Rank(s)");
#else
  report.add("Build", "MPI", "off");
#endif
#if defined(PEANO_WITH_OPENMP)
  report.add("Build", "OpenMP", number(tarch::multicore::Core::getInstance().getNumberOfThreads()) + " Thread(s)");
#else
  report.add("Build", "OpenMP", "off");
#endif
  report.add("Build", "CPU Features", number(tarch::detectCPUFeatures()));
  report.add("Build", "Stack Size", number(tarch::detectStackSizeLimit()));

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
    gpuBackend += ", " + number(tarch::accelerator::Device::getInstance().getNumberOfDevices()) + " Device(s)";
  }
#endif
  report.add("Build", "GPU Backend", gpuBackend);

  report.add(
    "Build",
    "Diagnostics",
    std::string("Trace=") + (PEANO_TRACE ? "on" : "off") + ", Assertions=" + (PEANO_ASSERTIONS ? "on" : "off") +
      ", Debug=" + (PEANO_DEBUG ? "on" : "off")
  );
  report.add("Build", "Git Branch", peano4::version::GitBranch);
  report.add("Build", "Git Commit Hash", peano4::version::GitCommitHash);
  report.add("Build", "Git Commit Message", peano4::version::GitCommitMessage);
  if (_repositories.getConfigurationInformation) {
    report.add("Build", "Generated With", _repositories.getConfigurationInformation());
  }
  if (not parameters.getConfigFilePath().empty()) {
    report.add("Build", "Runtime Configuration", "JSON file \"" + parameters.getConfigFilePath() + "\"");
  }

  // --- Domain & grid ---
  std::ostringstream domain;
  for (int d = 0; d < DIMENSIONS; ++d) {
    domain << (d == 0 ? "" : " x ") << "[" << _domainOffset[d] << ", " << (_domainOffset[d] + _domainSize[d]) << "]";
  }
  std::ostringstream periodic;
  const char*        axis = "xyz";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (_periodicBC[d]) {
      periodic << (periodic.str().empty() ? "" : ", ") << axis[d];
    }
  }
  std::ostringstream gridLevels;
  gridLevels << parameters.getGridLevels() << " .. " << parameters.getMaxGridLevels();
  std::ostringstream meshSize;
  meshSize << number(parameters.getMinAdmissiblePatchSize(_domainSize)) << " .. "
           << number(parameters.getMaxAdmissiblePatchSize(_domainSize));

  report.add("Domain & Grid", "Domain", domain.str());
  report.add("Domain & Grid", "Periodic Boundaries", periodic.str().empty() ? "none" : periodic.str());
  report.add("Domain & Grid", "Grid Levels (min .. max)", gridLevels.str());
  report.add("Domain & Grid", "Admissible Mesh Size (min .. max)", meshSize.str());

  // --- Time stepping ---
  const auto formatTime = [](double value) -> std::string {
    return value < std::numeric_limits<double>::max() / 10.0 ? number(value) : std::string("unbounded");
  };
  report.add("Time Stepping", "Min Terminal Time", formatTime(parameters.getMinTerminalTime()));
  report.add("Time Stepping", "Max Terminal Time", formatTime(parameters.getMaxTerminalTime()));
  report.add("Time Stepping", "First Plot Time Stamp", number(parameters.getFirstPlotTimeStamp()));
  report.add("Time Stepping", "Plot Snapshots", number(parameters.getNumberOfSnapshots()));
  report.add("Time Stepping", "Output Path", parameters.getOutputPath());
  report.add("Time Stepping", "Log Interval", number(parameters.getLogIntervalInSeconds()) + " s");

  // --- Load balancing ---
  report.add("Load Balancing", "Quality", number(parameters.getLoadBalancingQuality()));
  report.add("Load Balancing", "Min Tree Size", number(parameters.getMinSizeOfTree()));
  report.add("Load Balancing", "Max Trees per Rank", number(parameters.getMaxNumberOfTreesPerRank()));
  report.add("Load Balancing", "Target Device", number(parameters.getTargetDevice()));

  // --- MPI ---
  report.add("MPI", "Timeout Warning", number(parameters.getMPITimeOutWarning()) + " s");
  report.add("MPI", "Deadlock Timeout", number(parameters.getMPIDeadlockTimeOut()) + " s");
  report.add("MPI", "Deadlock Detection", parameters.isMPIDeadlockDetectionEnabled() ? "on" : "off");

  // --- Solvers (contributed by the project) ---
  if (_repositories.getConfigurationSummary) {
    for (const auto& [section, entries] : _repositories.getConfigurationSummary()) {
      for (const auto& [label, value] : entries) {
        report.add(section, label, value);
      }
    }
  }

  // The startup block is headed by the copyright banners (Peano always; a higher layer such as
  // ExaHyPE may contribute its own).
  std::vector<std::string> banners;
  banners.push_back("Peano 4 (C) www.peano-framework.org");
  if (_repositories.getCopyrightBanner) {
    banners.push_back(_repositories.getCopyrightBanner());
  }
  drawBox(banners, report.renderContent(), '=');
}

std::string runner::StatisticsLog::formatETA() const {
  if (not _repositories.getMinTimeStamp) {
    return "";
  }
  const auto elapsed         = _measurements.timeStep.getAccumulatedValue() + _measurements.plot.getAccumulatedValue();
  const auto minTimeStamp    = _repositories.getMinTimeStamp();
  const auto minTerminalTime = RuntimeParameters::getInstance().getMinTerminalTime();

  if (elapsed <= 0.0 or not std::isnormal(minTimeStamp) or minTimeStamp <= 0.0 or minTimeStamp >= minTerminalTime) {
    return "";
  }
  const double remaining = (minTerminalTime - minTimeStamp) * elapsed / minTimeStamp;
  if (remaining >= std::numeric_limits<double>::max() / 10.0) {
    return "";
  }
  const auto         seconds = static_cast<int>(remaining + 0.5);
  std::ostringstream result;
  result << "ETA " << seconds / 3600 << "h " << (seconds % 3600) / 60 << "m " << seconds % 60 << "s";
  return result.str();
}

runner::StatisticsReport runner::StatisticsLog::buildSimulationReport(bool includeSolverSections) {
  StatisticsReport report;
  for (const auto& provider : _baseProviders) {
    provider(report);
  }
  if (includeSolverSections) {
    for (const auto& provider : _solverProviders) {
      provider(report);
    }
  }
  return report;
}

void runner::StatisticsLog::refreshSimulationBox(
  const std::string& phase,
  double             wallClockSeconds,
  const std::string& progress,
  bool               includeSolverSections
) {
  const std::string left = phase + "   (" + formatWallClock(wallClockSeconds) + ")";
  const auto        body = buildSimulationReport(includeSolverSections).renderContent();

  // Right-align the progress string to the fixed box width; the left label stays put.
  std::string header = left;
  if (not progress.empty()) {
    const std::size_t used = displayWidth(left) + displayWidth(progress);
    const std::size_t gap  = (used + HeaderProgressGap <= BoxWidth) ? (BoxWidth - used) : HeaderProgressGap;
    header                 = left + std::string(gap, ' ') + progress;
  }
  drawBox({header}, body, '-');
}

void runner::StatisticsLog::recordStepTime(const std::string& observerName, double elapsedSeconds) {
  for (auto& [name, measurement] : _stepTimes) {
    if (name == observerName) {
      measurement.setValue(elapsedSeconds);
      return;
    }
  }
  _stepTimes.emplace_back(observerName, tarch::instrumentation::Measurement());
  _stepTimes.back().second.setValue(elapsedSeconds);
}

void runner::StatisticsLog::emitFinalSummary() {
  std::vector<std::pair<std::string, std::string>> entries;

  // Right-justify every duration to a common field so the decimal points, the "s" unit and the
  // trailing sweep counts line up across all rows; four decimal digits everywhere.
  const auto seconds = [](double value) {
    std::ostringstream result;
    result << std::fixed << std::setprecision(4) << std::setw(12) << value << " s";
    return result.str();
  };

  const auto totalTime = [&seconds](const tarch::instrumentation::Measurement& measurement, const char* count) {
    std::ostringstream value;
    value << seconds(measurement.getAccumulatedValue()) << "  (" << measurement.getNumberOfMeasurements() << " "
          << count << ")";
    return value.str();
  };

  // One uniform line per observer, keyed by its registered name, in first-seen order.
  // Load balancing is just another observer here - nothing is special-cased.
  for (const auto& [name, measurement] : _stepTimes) {
    entries.emplace_back("Total Time Spent in " + displayLabel(name), totalTime(measurement, "Sweep(s)"));
  }

  if (_measurements.timeStep.getNumberOfMeasurements() > 0) {
    entries.emplace_back("Average Time per Time Step", seconds(_measurements.timeStep.getAverageValue()));

    const auto leaves = static_cast<double>(loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto dt     = _measurements.timeStep.getAverageValue();
    if (dt > 0.0) {
      std::ostringstream performance;
      performance << rate(leaves / dt, "Cells/s");
      if (_repositories.getNumberOfDoFs) {
        performance << ", " << rate(static_cast<double>(_repositories.getNumberOfDoFs()) * leaves / dt, "DoF/s");
      }
      entries.emplace_back("Average Performance", performance.str());
    }
  }

  std::size_t labelWidth = 0;
  for (const auto& [label, value] : entries) {
    labelWidth = std::max(labelWidth, label.size());
  }
  std::vector<std::string> content;
  for (const auto& [label, value] : entries) {
    std::ostringstream line;
    line << "  " << std::left << std::setw(static_cast<int>(labelWidth)) << label << "  " << value;
    content.push_back(line.str());
  }

  drawBox({"Final Statistics"}, content, '=');
}

void runner::StatisticsLog::emitClosingBox(const std::string& message) { drawBox({message, "Goodbye"}, {}, '='); }
