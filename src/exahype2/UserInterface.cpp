#include "UserInterface.h"

#include "tarch/multicore/Core.h"
#include "tarch/mpi/Rank.h"

#include "tarch/logging/LogFilter.h"
#include "tarch/logging/LogFilterFileReader.h"

#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/orchestration/StrategyFactory.h"

#include "peano4/peano.h"

namespace {
  tarch::logging::Log _log( "exahype2" );

  void addBibTeXEntry() {
    peano4::addBibTeXEntry(
"## ExaHyPE 2 ##                                  \n"
"                                                                            \n"
"This code uses ExaHyPE in its second generation. The first generation of the "
"code was developed through an EU FET HPC project called ExaHyPE. This first  "
"generation was built on top of Peano in its third generation. The present    "
"code uses ExaHyPE 2 which is a complete rewrite built on top of Peano 4. We  "
"do not yet have a release paper for this second generation of ExaHyPE, and   "
"thus appreciate any citation of the original release paper\n"
"                                                                            \n"
"       @article{Reinarz:2020:ExaHyPE,   \n"
"         title = {ExaHyPE: An engine for parallel dynamically adaptive simulations of wave problems},   \n"
"         journal = {Computer Physics Communications},   \n"
"         volume = {254},   \n"
"         pages = {107251},   \n"
"         year = {2020},   \n"
"         issn = {0010-4655},   \n"
"         doi = {https://doi.org/10.1016/j.cpc.2020.107251},   \n"
"         url = {https://www.sciencedirect.com/science/article/pii/S001046552030076X},   \n"
"         author = {Anne Reinarz and Dominic E. Charrier and Michael Bader and Luke Bovard and Michael Dumbser and Kenneth Duru and Francesco Fambri and Alice-Agnes Gabriel and Jean-Matthieu Gallard and Sven K\"oppel and Lukas Krenz and Leonhard Rannabauer and Luciano Rezzolla and Philipp Samfass and Maurizio Tavelli and Tobias Weinzierl},   \n"
"         keywords = {Hyperbolic, PDE, ADER-DG, Finite volumes, AMR, MPI, TBB, MPI+X},   \n"
"         abstract = {ExaHyPE (An Exascale Hyperbolic PDE Engine) is a software engine for solving systems of first-order hyperbolic partial differential equations (PDEs). Hyperbolic PDEs are typically derived from the conservation laws of physics and are useful in a wide range of application areas. Applications powered by ExaHyPE can be run on a student's laptop, but are also able to exploit thousands of processor cores on state-of-the-art supercomputers. The engine is able to dynamically increase the accuracy of the simulation using adaptive mesh refinement where required. Due to the robustness and shock capturing abilities of ExaHyPE's numerical methods, users of the engine can simulate linear and non-linear hyperbolic PDEs with very high accuracy. Users can tailor the engine to their particular PDE by specifying evolved quantities, fluxes, and source terms. A complete simulation code for a new hyperbolic PDE can often be realised within a few hours - a task that, traditionally, can take weeks, months, often years for researchers starting from scratch. In this paper, we showcase ExaHyPE's workflow and capabilities through real-world scenarios from our two main application areas: seismology and astrophysics.   \n"
"           Program summary   \n"
"           Program title: ExaHyPE-Engine Program Files doi: http://dx.doi.org/10.17632/6sz8h6hnpz.1 Licensing provisions: BSD 3-clause Programming languages: C++, Python, Fortran Nature of Problem: The ExaHyPE PDE engine offers robust algorithms to solve linear and non-linear hyperbolic systems of PDEs written in first order form. The systems may contain both conservative and non-conservative terms. Solution method: ExaHyPE employs the discontinuous Galerkin (DG) method combined with explicit one-step ADER (arbitrary high-order derivative) time-stepping. An a-posteriori limiting approach is applied to the ADER-DG solution, whereby spurious solutions are discarded and recomputed with a robust, patch-based finite volume scheme. ExaHyPE uses dynamical adaptive mesh refinement to enhance the accuracy of the solution around shock waves, complex geometries, and interesting features. \n"
"         }   \n"
"       } \n"
"\n\n\n\n" );

    #if defined(SharedMemoryParallelisation)
    peano4::addBibTeXEntry(
"## Shared memory parallelisation ##                                  \n"
"                                                                            \n"
"Peano relies on a mixture of classic domain decomposition and task-based  "
"parallelism. The domain decomposition provides the baseline performance,  "
"and the tasking adds the big flexibility and scalability gain on top. The "
"key publication discussing the overall idea and algorithmic ingredients is "
"the SISC paper\n"
"                                                                            \n"
"       @article{Charrier:2020:Enclave,\n"
"         author = {Charrier, Dominic Etienne and Hazelwood, Benjamin and Weinzierl, Tobias},\n"
"         title = {Enclave Tasking for DG Methods on Dynamically Adaptive Meshes},\n"
"         journal = {SIAM Journal on Scientific Computing},\n"
"         volume = {42},\n"
"         number = {3},\n"
"         pages = {C69-C96},\n"
"         year = {2020},\n"
"         doi = {10.1137/19M1276194},\n"
"         URL = {https://doi.org/10.1137/19M1276194},\n"
"         eprint = {https://doi.org/10.1137/19M1276194}\n"
"       }\n"
"\n\n\n\n" );
    #endif
  }
}




void exahype2::setDefaultLogStatements() {
  addBibTeXEntry();

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
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetTrace,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
      tarch::logging::LogFilter::FilterListEntry::BlackListEntry,
      tarch::logging::LogFilter::FilterListEntry::AlwaysOn
  ));
  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
      tarch::logging::LogFilter::FilterListEntry::TargetInfo,
      tarch::logging::LogFilter::FilterListEntry::AnyRank,
      "exahype2",
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


void exahype2::printUsage(char** argv) {
  std::cout << "Usage: ./" << argv[0] << " [options]" << std::endl <<
"ExaHyPE 2 - an Exascale Hyperbolic PDE Engine (version 2) \n\
www.peano-framework.org \n\
\n \
Options: \n\
  -h, --help                 Display help on commandline options. \n\
  --threads <no>             Specify how many threads to use (per rank). Option \n\
                             has no meaning if code base has not been \n\
                             translated with shared memory support. Not all \n\
                             runtimes allow you to set the thread count via the \n\
                             code. \n\
  --log-filter-file <file>   Specify which log filter file to use. Default file \n\
                             is exahype.log-filter \n\
  --timeout <t>              Set timeout. t is given in seconds and can be 0 to \n\
                             switch timeouts off. \n\
  --threading-model <t>      Set threading model. \n\
\n\n\n\
Supported threading models: " << tarch::multicore::orchestration::getListOfRealisations() << std::endl;
}


bool exahype2::parseCommandLineArguments(int argc, char** argv) {
  addBibTeXEntry();

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

