// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2.h"

#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"
#include "tarch/mpi/Rank.h"

namespace {
  tarch::logging::Log _log("exahype2");
} // namespace

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
