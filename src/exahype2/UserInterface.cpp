#include "UserInterface.h"

#include "tarch/multicore/Core.h"
#include "tarch/mpi/Rank.h"

#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"


namespace {
  tarch::logging::Log _log( "exahype2" );
}


void exahype2::setDefaultLogStatements() {
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
       tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
     "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));
}


void exahype2::printUsage(char** argv) {
  std::cout << "Usage: ./" << argv[0] << " [options]" << std::endl <<
"ExaHyPE 2 - an Exascale Hyperbolic PDE Engine (version 2) \n\
www.peano-framework.org \n\
\n \
Options: \n\
  -h, --help                 Display help on commandline options. \n\
  --threads <no>             Specify how many threads to use (per rank). Option \n\
                             has no meaning if code base has not been \n\
                             translated with shared memory support. \n\
  --log-filter-file <file>   Specify which log filter file to use. Default file \n\
                             is exahype.log-filter \n\
  --timeout <t>              Set timeout. t is given in seconds and can be 0 to \n\
                             switch timeouts off. \n\
";
}


bool exahype2::parseCommandLineArguments(int argc, char** argv) {
  if (
    (argc==2 and std::string(argv[1]).compare( "--help" )!=std::string::npos)
    or
    (argc==2 and std::string(argv[1]).compare( "-h" )!=std::string::npos)
    or
    (argc%2!=1)
  ) {{
    printUsage(argv);
    return false;
  }}

  std::string logFilterFile = "exahype.log-filter";
  int         cores         = tarch::multicore::Core::UseDefaultNumberOfThreads;

  for (int argument=1; argument<argc; argument+=2) {
    std::string select = argv[argument];

    if ( select.compare( "--threads" ) ==0 ) {
      cores = std::atoi(argv[argument+1]);
      tarch::multicore::Core::getInstance().configure( cores );
      logInfo( "parseCommandLineArguments(...)", "manually reset number of used cores to " << cores );
    }
    else if ( select.compare( "--log-filter-file" ) == 0 ) {
      logFilterFile = argv[argument+1];
    }
    else if ( select.compare( "--timeout" ) == 0 ) {
      int timeout = std::atoi(argv[argument+1]);
      tarch::mpi::Rank::getInstance().setDeadlockTimeOut( timeout );
      logInfo( "parseCommandLineArguments(...)", "manually set timeout to " << timeout );
    }
    else {
      printUsage(argv);
      return false;
    }
  }

  if ( not tarch::logging::LogFilterFileReader::parsePlainTextFile( logFilterFile ) ) {
    logWarning( "main()", "no exahype.log-filter file found or file has been corrupted. Use default logging configuration" );
    setDefaultLogStatements();
  }

  return true;
}

