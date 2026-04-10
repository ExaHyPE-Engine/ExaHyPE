// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2.h"

#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"
#include "tarch/mpi/Rank.h"
#include "tarch/reader/JSONReader.h"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
  tarch::logging::Log   _log("exahype2");
  constexpr const char* DefaultConfigFile = "./exahype.json";

  std::string normaliseOutputPath(const std::string& path) {
    if (path.empty()) {
      return "./";
    }
    return path.back() == '/' ? path : path + "/";
  }

  tarch::la::Vector<DIMENSIONS, double> toDomainVector(const std::vector<double>& values, const std::string& key) {
    if (values.size() < DIMENSIONS) {
      std::ostringstream message;
      message
        << "JSON key \"" << key << "\" must contain at least " << DIMENSIONS << " entries, but got " << values.size()
        << ".";
      throw std::runtime_error(message.str());
    }

    tarch::la::Vector<DIMENSIONS, double> result;
    for (int i = 0; i < DIMENSIONS; ++i) {
      result[i] = values[i];
    }
    return result;
  }

  std::bitset<DIMENSIONS> toPeriodicBC(const std::vector<bool>& values, const std::string& key) {
    if (values.size() < DIMENSIONS) {
      std::ostringstream message;
      message
        << "JSON key \"" << key << "\" must contain at least " << DIMENSIONS << " entries, but got " << values.size()
        << ".";
      throw std::runtime_error(message.str());
    }

    std::bitset<DIMENSIONS> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = values[d];
    }
    return result;
  }
} // namespace

namespace {
  void storeRuntimeConfiguration(const exahype2::RuntimeConfiguration& configuration) {
    std::fill_n(exahype2::internal::RuntimeParametersStorage.domainOffset, 3, 0.0);
    std::fill_n(exahype2::internal::RuntimeParametersStorage.domainSize, 3, 1.0);
    std::fill_n(exahype2::internal::RuntimeParametersStorage.periodicBC, 3, false);

    for (int d = 0; d < DIMENSIONS; ++d) {
      exahype2::internal::RuntimeParametersStorage.domainOffset[d] = configuration.domainOffset[d];
      exahype2::internal::RuntimeParametersStorage.domainSize[d]   = configuration.domainSize[d];
      exahype2::internal::RuntimeParametersStorage.periodicBC[d]   = configuration.periodicBC[d];
    }
    exahype2::internal::RuntimeParametersStorage.minTerminalTime    = configuration.minTerminalTime;
    exahype2::internal::RuntimeParametersStorage.maxTerminalTime    = configuration.maxTerminalTime;
    exahype2::internal::RuntimeParametersStorage.firstPlotTimeStamp = configuration.firstPlotTimeStamp;
    exahype2::internal::RuntimeParametersStorage.timeInBetweenPlots = configuration.timeInBetweenPlots;
    exahype2::internal::RuntimeParametersStorage.plotOutputPath     = normaliseOutputPath(configuration.plotOutputPath);
  }
} // namespace

namespace exahype2::internal {
  TARGET_DECLARE
  RuntimeParameters RuntimeParametersStorage;
  TARGET_DECLARE_END
} // namespace exahype2::internal

std::string exahype2::extractConfigFileFromCommandLine(int& argc, char** argv) {
  std::string configFilePath;

  int writeIndex = 1;
  for (int readIndex = 1; readIndex < argc; ++readIndex) {
    const std::string argument = argv[readIndex];
    if (argument == "--config-file") {
      if (readIndex + 1 >= argc) {
        throw std::runtime_error("Missing value for --config-file.");
      }
      configFilePath = argv[readIndex + 1];
      ++readIndex;
    } else {
      argv[writeIndex] = argv[readIndex];
      ++writeIndex;
    }
  }

  argc       = writeIndex;
  argv[argc] = nullptr;

  if (!configFilePath.empty()) {
    return configFilePath;
  }

  if (std::filesystem::exists(DefaultConfigFile)) {
    return DefaultConfigFile;
  }

  return "";
}

exahype2::RuntimeConfiguration exahype2::loadRuntimeConfiguration(
  const std::string&          configFilePath,
  const RuntimeConfiguration& fallbackConfiguration
) {
  if (configFilePath.empty()) {
    storeRuntimeConfiguration(fallbackConfiguration);
    return fallbackConfiguration;
  }

  tarch::reader::JSONReader reader(configFilePath);

  RuntimeConfiguration result = fallbackConfiguration;
  result.domainOffset         = toDomainVector(reader.getDoubleArray("domain.offset"), "domain.offset");
  result.domainSize           = toDomainVector(reader.getDoubleArray("domain.size"), "domain.size");
  result.minTerminalTime      = reader.getDouble("simulation.minTerminalTime");
  result.maxTerminalTime      = reader.getDouble("simulation.maxTerminalTime");
  result.firstPlotTimeStamp   = reader.getDouble("plot.firstTimeStamp");
  result.timeInBetweenPlots   = reader.getDouble("plot.timeInBetweenPlots");
  result.plotOutputPath       = reader.getString("plot.outputPath");
  result.periodicBC           = toPeriodicBC(reader.getBooleanArray("boundary.periodic"), "boundary.periodic");

  storeRuntimeConfiguration(result);
  return result;
}

void exahype2::writeCopyrightMessage() {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo("writeCopyrightMessage()", "ExaHyPE 2 (C) www.peano-framework.org");
  }
}

void exahype2::setDefaultLogStatements() {
  tarch::logging::LogFilter::getInstance().clearFilterList();
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );


  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );

  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
  tarch::logging::LogFilter::getInstance().addFilterListEntry(
    tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
    )
  );
}
