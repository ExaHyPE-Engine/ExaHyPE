#include "UserInterface.h"

#include "tarch/multicore/Core.h"
#include "tarch/accelerator/Device.h"
#include "tarch/mpi/Rank.h"

#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"

#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/orchestration/StrategyFactory.h"

#include "peano4/peano.h"

namespace {
  tarch::logging::Log _log( "swift2" );

  bool enableDynamicLoadBalancing = true;
}


bool swift2::commandlinesettings::enableDynamicLoadBalancing() {
  return ::enableDynamicLoadBalancing;
}


void swift2::setDefaultLogStatements() {
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "peano4",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
       tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
     "tarch",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "tarch",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "swift2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "swift2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "swift2",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetDebug,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "toolbox",
      tarch::logging::LogFilter::FilterListEntry::WhiteListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
}


void swift2::printUsage(char** argv) {
  std::cout << "Usage: ./" << argv[0] << " [options]" << std::endl <<
"SWIFT 2 - an experimental rewrite of SPH With Inter-dependent Fine-grained Tasking \n\
Original code: https://swift.dur.ac.uk/ \n\
\n \
Options: \n\
  -h, --help                Display help on commandline options. \n\
  --threads no              Specify how many threads to use (per rank). Option \n\
                            has no meaning if code base has not been \n\
                            translated with shared memory support. Not all \n\
                            runtimes allow you to set the thread count via the \n\
                            code. \n\
  --gpus filter             Specify which GPUs to use. Option has no meaning \n\
                            if code base has not been translated with gpu \n\
                            support. Pass in comma-separated list (3,4,9) to \n\
                            select which GPUs to use. If argument is empty, ExaHyPE \n\
                            may use all GPUs. If you want to use all GPUs, hand \n\
                            in all as argument. \n\
  --log-filter-file file    Specify which log filter file to use. Default file \n\
                            is exahype.log-filter \n\
  --timeout t               Set timeout. t is given in seconds and can be 0 to \n\
                            switch timeouts off. \n\
  --threading-model t       Set threading model. \n\
  --dynamic-load-balancing [on|off] Disable or enable dynamic load balancing. Pass \n\
                            either on or off. The default is on. \n\
\n\n\n\
Supported threading models: " << tarch::multicore::orchestration::getListOfRealisations() << std::endl;
}


bool swift2::parseCommandLineArguments(int argc, char** argv) {
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

  std::string   logFilterFile = "swift2.log-filter";
  int           cores         = tarch::multicore::Core::UseDefaultNumberOfThreads;
  std::set<int> gpuDevices;

  for (int argument=1; argument<argc; argument+=2) {
    std::string select = argv[argument];

    if ( select.compare( "--threads" ) ==0 ) {
      cores = std::atoi(argv[argument+1]);
      logInfo( "parseCommandLineArguments(...)", "manually reset number of used cores to " << cores );
    }
    else if ( select.compare( "--gpus" ) ==0 ) {
      std::string specifier = argv[argument+1];
      std::string delimiter = ",";
      if ( specifier.compare( "all" ) ==0 ) {
        logInfo( "parseCommandLineArguments(...)", "enable all devices" );
      }
      else if (specifier.find( delimiter ) == std::string::npos) {
        int gpu = std::atoi(specifier.c_str());
        gpuDevices.insert(gpu);
        logInfo( "parseCommandLineArguments(...)", "enable only GPU " << gpu );
      }
      else {
        size_t pos = 0;
        std::string token;
        while ((pos = specifier.find(delimiter)) != std::string::npos) {
          token = specifier.substr(0, pos);
          int gpu = std::atoi(token.c_str());
          gpuDevices.insert(gpu);
          specifier.erase(0, pos + delimiter.length());
        }

        logInfo( "parseCommandLineArguments(...)", "manually set GPU filter with " << gpuDevices.size() << " devices enabled" );
      }
    }
    else if ( select.compare( "--log-filter-file" ) == 0 ) {
      logFilterFile = argv[argument+1];
    }
    else if ( select.compare( "--timeout" ) == 0 ) {
      int timeout = std::atoi(argv[argument+1]);
      tarch::mpi::Rank::getInstance().setDeadlockTimeOut( timeout );
      logInfo( "parseCommandLineArguments(...)", "manually set timeout to " << timeout );
    }
    else if ( select.compare( "--threading-model" ) == 0 ) {
      auto* realisation = tarch::multicore::orchestration::parseRealisation( argv[argument+1] );
      if (realisation!=nullptr) {
        tarch::multicore::setOrchestration(realisation);
      }
      else {
        logError( "parseCommandLineArguments(...)", "was not able to set threading strategy " << argv[argument+1] );
        return false;
      }
    }
    else if ( select.compare( "--dynamic-load-balancing" ) == 0 ) {
      std::string value = argv[argument+1];
      if (value==std::string("on")) {
        enableDynamicLoadBalancing = true;
      }
      else if (value==std::string("off")) {
        enableDynamicLoadBalancing = false;
      }
      else {
        logError( "parseCommandLineArguments(...)", "load balancing value has to be on or off. Value had been " << value );
        return false;
      }
    }
    else {
      printUsage(argv);
      return false;
    }
  }

  tarch::multicore::Core::getInstance().configure( cores );
  tarch::accelerator::Device::getInstance().configure( gpuDevices );

  if ( not tarch::logging::LogFilterFileReader::parsePlainTextFile( logFilterFile ) ) {
    logWarning( "main()", "no exahype.log-filter file found or file has been corrupted. Use default logging configuration" );
    setDefaultLogStatements();
  }

  return true;
}

