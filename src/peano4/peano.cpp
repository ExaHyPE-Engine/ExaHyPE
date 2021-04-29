#include "peano.h"

#include "peano4/utils/Loop.h"
#include "peano4/parallel/Node.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/tarch.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


#include <list>
#include <fstream>



/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef CompilerHasUTSName
#include <sys/utsname.h>
#endif


namespace {
  std::set< std::string > bibTeXEntries;

  void addDefaultBibTeXEntries() {
    peano4::addBibTeXEntry(
"## Peano 4 ##                                  \n"
"                                                                            \n"
"Peano's core ideas, algorithms and (usage) vision are discussed in the paper "
"below. There is an earlier publication about Peano from SISC (Weinzierl and Mehl; see below) "
"which discusses the core algorithms in even greater detail. This older paper "
"can be seen as release paper for previous Peano code generations. The TOMS "
"paper below in contrast describes the 4th generation of Peano and puts    "
"more emphasis on the software design.                                   \n"
"                                                                            \n"
"        @article{Weinzierl:2019:Peano,                    \n"
"          author = {Weinzierl, Tobias},                    \n"
"          title = {The Peano Software—Parallel, Automaton-Based, Dynamically Adaptive Grid Traversals},                    \n"
"          year = {2019},                    \n"
"          issue_date = {June 2019},                    \n"
"          publisher = {Association for Computing Machinery},                    \n"
"          address = {New York, NY, USA},                    \n"
"          volume = {45},                    \n"
"          number = {2},                    \n"
"          issn = {0098-3500},                    \n"
"          url = {https://doi.org/10.1145/3319797},                    \n"
"          doi = {10.1145/3319797},                    \n"
"          abstract = {We discuss the design decisions, design alternatives, and rationale behind the third generation of Peano, a framework for dynamically adaptive Cartesian meshes derived from spacetrees. Peano ties the mesh traversal to the mesh storage and supports only one element-wise traversal order resulting from space-filling curves. The user is not free to choose a traversal order herself. The traversal can exploit regular grid subregions and shared memory as well as distributed memory systems with almost no modifications to a serial application code. We formalize the software design by means of two interacting automata—one automaton for the multiscale grid traversal and one for the application-specific algorithmic steps. This yields a callback-based programming paradigm. We further sketch the supported application types and the two data storage schemes realized before we detail high-performance computing aspects and lessons learned. Special emphasis is put on observations regarding the used programming idioms and algorithmic concepts. This transforms our report from a “one way to implement things” code description into a generic discussion and summary of some alternatives, rationale, and design decisions to be made for any tree-based adaptive mesh refinement software.},                    \n"
"          journal = {ACM Trans. Math. Softw.},                    \n"
"          month = apr,                    \n"
"          articleno = {14},                    \n"
"          numpages = {41},                    \n"
"          keywords = {Software, spacetree, adaptive mesh refinement, parallel multiscale grid traversal}                    \n"
"        }                    \n"
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


    #if defined(Parallel)
    peano4::addBibTeXEntry(
"## Distributed memory parallelisation ##                                  \n"
"                                                                            \n"
"Peano's distributed memory parallelisation is based upon plain space-filling "
"curves which are used first to split the cells among the ranks, and, after   "
"that, to split the cells into chunks and deploy them onto cores (if shared   "
"memory is used). There is no particularly novel contribution/paper on this   "
"aspect of the code. The paper that is (algorithmically) most interesting is  "
"an old EuroPar publication:                                                 \n"
"                                                                            \n"
"       @InProceedings{Bungartz:2006:Parallelisation, \n"
"         author={Bungartz, Hans-Joachim and Mehl, Miriam and Weinzierl, Tobias}, \n"
"         editor={Nagel, Wolfgang E. and Walter, Wolfgang V. and Lehner, Wolfgang}, \n"
"         title={A Parallel Adaptive Cartesian PDE Solver Using Space--Filling Curves},  \n"
"         booktitle={Euro-Par 2006 Parallel Processing}, \n"
"         year={2006}, \n"
"         publisher={Springer Berlin Heidelberg}, \n"
"         address={Berlin, Heidelberg}, \n"
"         pages={1064--1074}, \n"
"         abstract={In this paper, we present a parallel multigrid PDE solver working on adaptive hierarchical cartesian grids. The presentation is restricted to the linear elliptic operator of second order, but extensions are possible and have already been realised as prototypes. Within the solver the handling of the vertices and the degrees of freedom associated to them is implemented solely using stacks and iterates of a Peano space--filling curve. Thus, due to the structuredness of the grid, two administrative bits per vertex are sufficient to store both geometry and grid refinement information. The implementation and parallel extension, using a space--filling curve to obtain a load balanced domain decomposition, will be formalised. In view of the fact that we are using a multigrid solver of linear complexity {\$}{\backslash}mathcal{\{}O{\}}(n){\$}, it has to be ensured that communication cost and, hence, the parallel algorithm's overall complexity do not exceed this linear behaviour.}, \n"
"         isbn={978-3-540-37784-9} \n"
"       } \n"
"\n\n\n\n" );
    #endif

    peano4::addBibTeXEntry(
"## Memory footprint ##                                  \n"
"                                                                            \n"
"Peano models its data structures via a tool called DaStGen. The actual DaStGen "
"version in use is the second generation of DaStGen which is integrated into "
"LLVM, e.g. The first generation of DaStGen has been a stand-alone Java tool "
"which serves as a precompiler. As DaStGen 2 is not published yet, we appreciate "
"a citation of the two original DaStGen papers when you discuss the memory needs: \n"
"                                                                            \n"
"        @article{Weinzierl:2019:Peano,                    \n"
"          author = {Weinzierl, Tobias},                    \n"
"          title = {The Peano Software—Parallel, Automaton-Based, Dynamically Adaptive Grid Traversals},                    \n"
"          year = {2019},                    \n"
"          issue_date = {June 2019},                    \n"
"          publisher = {Association for Computing Machinery},                    \n"
"          address = {New York, NY, USA},                    \n"
"          volume = {45},                    \n"
"          number = {2},                    \n"
"          issn = {0098-3500},                    \n"
"          url = {https://doi.org/10.1145/3319797},                    \n"
"          doi = {10.1145/3319797},                    \n"
"          abstract = {We discuss the design decisions, design alternatives, and rationale behind the third generation of Peano, a framework for dynamically adaptive Cartesian meshes derived from spacetrees. Peano ties the mesh traversal to the mesh storage and supports only one element-wise traversal order resulting from space-filling curves. The user is not free to choose a traversal order herself. The traversal can exploit regular grid subregions and shared memory as well as distributed memory systems with almost no modifications to a serial application code. We formalize the software design by means of two interacting automata—one automaton for the multiscale grid traversal and one for the application-specific algorithmic steps. This yields a callback-based programming paradigm. We further sketch the supported application types and the two data storage schemes realized before we detail high-performance computing aspects and lessons learned. Special emphasis is put on observations regarding the used programming idioms and algorithmic concepts. This transforms our report from a “one way to implement things” code description into a generic discussion and summary of some alternatives, rationale, and design decisions to be made for any tree-based adaptive mesh refinement software.},                    \n"
"          journal = {ACM Trans. Math. Softw.},                    \n"
"          month = apr,                    \n"
"          articleno = {14},                    \n"
"          numpages = {41},                    \n"
"          keywords = {Software, spacetree, adaptive mesh refinement, parallel multiscale grid traversal}                    \n"
"        }                    \n"
"\n\n\n\n" );

    peano4::addBibTeXEntry(
"## Memory footprint ##                                  \n"
"                                                                            \n"
"Peano models its data structures via a tool called DaStGen. The actual DaStGen "
"version in use is the second generation of DaStGen which is integrated into "
"LLVM, e.g. The first generation of DaStGen has been a stand-alone Java tool "
"which serves as a precompiler. As DaStGen 2 is not published yet, we appreciate "
"a citation of the two original DaStGen papers when you discuss the memory needs: \n"
"                                                                            \n"
"        @InProceedings{Bungartz:2008:DaStGen, \n"
"          author={Bungartz, Hans--Joachim and Eckhardt, Wolfgang and Mehl, Miriam and Weinzierl, Tobias}, \n"
"          editor={Bubak, Marian and van Albada, Geert Dick and Dongarra, Jack and Sloot, Peter M. A.}, \n"
"          title={DaStGen---A Data Structure Generator for Parallel C++ HPC Software}, \n"
"          booktitle={Computational Science -- ICCS 2008}, \n"
"          year=2008, \n"
"          publisher={Springer Berlin Heidelberg}, \n"
"          address={Berlin, Heidelberg}, \n"
"          pages={213--222}, \n"
"          abstract={Simulation codes often suffer from high memory requirements. This holds in particular if they are memory-bounded, and, with multicore systems coming up, the problem will become even worse as more and more cores have to share the memory connections. To optimise data structures with respect to memory manually is error-prone and cumbersome.}, \n"
"          isbn={978-3-540-69389-5} \n"
"        } \n\n\n"
"        @article{Bungartz:2010:DaStGen, \n"
"          title = {A precompiler to reduce the memory footprint of multiscale PDE solvers in C++}, \n"
"          journal = {Future Generation Computer Systems}, \n"
"          volume = {26}, \n"
"          number = {1}, \n"
"          pages = {175-182}, \n"
"          year = {2010}, \n"
"          issn = {0167-739X}, \n"
"          doi = {https://doi.org/10.1016/j.future.2009.05.011}, \n"
"          url = {https://www.sciencedirect.com/science/article/pii/S0167739X09000673}, \n"
"          author = {H.-J. Bungartz and W. Eckhardt and T. Weinzierl and C. Zenger}, \n"
"          keywords = {Data compaction and compression, Code generation, Multigrid and multilevel methods}, \n"
"          abstract = {A PDE solver’s value is increasingly co-determined by its memory footprint, as the increase of computational multicore power overtakes the memory access speed, and as memory restricts the maximum experiment size. Tailoring a code to require less memory is technical challenging, error-prone, and hardware-dependent. Object-oriented code typically consumes much memory, though developers favour such high-level languages offering meaningful models and good maintainability. We augment the language C++ with new keywords branding records to be memory-critical. Our precompiler DaStGen then transforms this augmented specification into plain C++ optimised for low memory requirements. Hereby, it encodes multiple attributes with fixed range within one variable, and it reduces the number of bits per floating point value. The tool also generates one user-defined MPI data type per class and, thus, facilitates the construction of parallel codes with small messages.} \n"
"        } \n"
"\n\n\n\n" );

    peano4::addBibTeXEntry(
"## Memory access characteristics ##                                  \n"
"                                                                            \n"
"Peano simulations usually achieve quite high cache hit rates and are memory "
"modest even for extreme AMR. The reason for these properties is the absence "
"of explicit AMR administration data structures and the usage of stacks as "
"omnipresent storage container. This idea is explained en detail in the SISC "
"publication below, and the property is typically easily confirmed through "
"performance counters for any code built on top of Peano. Note that the paper "
"studies/introduces the third generation of Peano, while the present experiments "
"are done with Peano 4. Peano 4 however uses stacks everywhere, whereas Peano 3"
"still had some non-stack meta data structures. If you discuss memory access "
"characteristics, the SISC paper thus is valid even if it refers to an older "
"code generation: \n"
"                                                                            \n"
"        @article{10.1137/100799071, \n"
"          author = {Weinzierl, Tobias and Mehl, Miriam}, \n"
"          title = {Peano—A Traversal and Storage Scheme for Octree-Like Adaptive Cartesian Multiscale Grids}, \n"
"          year = {2011}, \n"
"          issue_date = {September 2011}, \n"
"          publisher = {Society for Industrial and Applied Mathematics}, \n"
"          address = {USA}, \n"
"          volume = {33}, \n"
"          number = {5}, \n"
"          issn = {1064-8275}, \n"
"          url = {https://doi.org/10.1137/100799071}, \n"
"          doi = {10.1137/100799071}, \n"
"          abstract = {Almost all approaches to solving partial differential equations (PDEs) are based upon a spatial discretization of the computational domain—a grid. This paper presents an algorithm to generate, store, and traverse a hierarchy of $d$-dimensional Cartesian grids represented by a $(k=3)$-spacetree, a generalization of the well-known octree concept, and it also shows the correctness of the approach. These grids may change their adaptive structure throughout the traversal. The algorithm uses $2d+4$ stacks as data structures for both cells and vertices, and the storage requirements for the pure grid reduce to one bit per vertex for both the complete grid connectivity structure and the multilevel grid relations. Since the traversal algorithm uses only stacks, the algorithm's cache hit rate is continually higher than 99.9 percent, and the runtime per vertex remains almost constant; i.e., it does not depend on the overall number of vertices or the adaptivity pattern. We use the algorithmic approach as the fundamental concept for a mesh management for $d$-dimensional PDEs and for a matrix-free PDE solver represented by a compact discrete $3^d$-point operator. In the latter case, one can implement a Jacobi smoother, a Krylov solver, or a geometric multigrid scheme within the presented traversal scheme which inherits the low memory requirements and the good memory access characteristics directly.}, \n"
"          journal = {SIAM J. Sci. Comput.}, \n"
"          month = oct, \n"
"          pages = {2732--2760}, \n"
"          numpages = {29}, \n"
"          keywords = {octree, cache efficiency, multiscale, space-filling curve, adaptive Cartesian grid, spacetree, partial differential equation} \n"
"        } \n"
"\n\n\n\n" );

    #if defined(SharedOMP)
    peano4::addBibTeXEntry(
"## User-level tasking ##                                  \n"
"                                                                            \n"
"Your Peano run uses a wrapper around OpenMP to obtain a high task efficiency. "
"The wrapper can be read as user-level threading implemented on top of OpenMP's "
"tasking mechanism. It is described in "
"                                                                            \n"
"       @article{Schulz:2021:Tasking,   \n"
"         title = {ExaHyPE: An engine for parallel dynamically adaptive simulations of wave problems},   \n"
"         journal = {Computer Physics Communications},   \n"
"         volume = {254},   \n"
"         pages = {107251},   \n"
"         year = {2020},   \n"
"         issn = {0010-4655},   \n"
"         doi = {https://doi.org/10.1016/j.cpc.2020.107251},   \n"
"         url = {https://www.sciencedirect.com/science/article/pii/S001046552030076X},   \n"
"         author = {Anne Reinarz and Dominic E. Charrier and Michael Bader and Luke Bovard and Michael Dumbser and Kenneth Duru and Francesco Fambri and Alice-Agnes Gabriel and Jean-Matthieu Gallard and Sven Köppel and Lukas Krenz and Leonhard Rannabauer and Luciano Rzzolla and Philipp Samfass and Maurizio Tavelli and Tobias Weinzierl},   \n"
"         keywords = {Hyperbolic, PDE, ADER-DG, Finite volumes, AMR, MPI, TBB, MPI+X},   \n"
"         abstract = {ExaHyPE (An Exascale Hyperbolic PDE Engine) is a software engine for solving systems of first-order hyperbolic partial differential equations (PDEs). Hyperbolic PDEs are typically derived from the conservation laws of physics and are useful in a wide range of application areas. Applications powered by ExaHyPE can be run on a student's laptop, but are also able to exploit thousands of processor cores on state-of-the-art supercomputers. The engine is able to dynamically increase the accuracy of the simulation using adaptive mesh refinement where required. Due to the robustness and shock capturing abilities of ExaHyPE's numerical methods, users of the engine can simulate linear and non-linear hyperbolic PDEs with very high accuracy. Users can tailor the engine to their particular PDE by specifying evolved quantities, fluxes, and source terms. A complete simulation code for a new hyperbolic PDE can often be realised within a few hours - a task that, traditionally, can take weeks, months, often years for researchers starting from scratch. In this paper, we showcase ExaHyPE's workflow and capabilities through real-world scenarios from our two main application areas: seismology and astrophysics.   \n"
"           Program summary   \n"
"           Program title: ExaHyPE-Engine Program Files doi: http://dx.doi.org/10.17632/6sz8h6hnpz.1 Licensing provisions: BSD 3-clause Programming languages: C++, Python, Fortran Nature of Problem: The ExaHyPE PDE engine offers robust algorithms to solve linear and non-linear hyperbolic systems of PDEs written in first order form. The systems may contain both conservative and non-conservative terms. Solution method: ExaHyPE employs the discontinuous Galerkin (DG) method combined with explicit one-step ADER (arbitrary high-order derivative) time-stepping. An a-posteriori limiting approach is applied to the ADER-DG solution, whereby spurious solutions are discarded and recomputed with a robust, patch-based finite volume scheme. ExaHyPE uses dynamical adaptive mesh refinement to enhance the accuracy of the solution around shock waves, complex geometries, and interesting features. \n"
"         }   \n"
"       } \n"
"\n\n\n\n" );
    #endif
  }
}


void peano4::addBibTeXEntry( const std::string& newEntry ) {
  bibTeXEntries.insert( newEntry );
}


void peano4::writeBibTeXMessage() {
  static bool wrote = false;
  if (
    not wrote
    and
    tarch::mpi::Rank::getInstance().getRank() == tarch::mpi::Rank::getGlobalMasterRank()
  ) {
    addDefaultBibTeXEntries();

    const std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();

    std::ostringstream filename;
    filename << "readme";


    #ifdef CompilerHasUTSName
    utsname* utsdata = new utsname();
    assertion( utsdata!=NULL );
    uname(utsdata);
    filename << "." << utsdata->nodename;
    delete utsdata;
    #endif

    filename << ".md";

    std::ofstream file( filename.str() );
    file << "# Peano 4 - Meta information #"
         << std::endl << std::endl;

    file <<
"                                                                            \n"
"This experiment did rely on Peano 4 or a toolbox created on top of Peano.   "
"If you publish results obtained with this source code, we would appreciate  "
"if you could point to the project homepage                                  \n"
"                                                                            \n"
"http://www.peano-framework.org/                                             \n"
"                                                                            \n"
"and cite the appropriate papers from below. \n\n\n";

    for (auto& p: bibTeXEntries) {
      file << p << std::endl << std::endl;
    }
    file.close();
    std::cout << "Peano 4 terminated successfully. See " << filename.str() << " for run summary" << std::endl;
  }
  wrote = true;
}


void peano4::writeCopyrightMessage() {
  static bool wrote = false;
  if (
    not wrote and tarch::mpi::Rank::getInstance().getRank() == tarch::mpi::Rank::getGlobalMasterRank() ) {
    std::cout << "Peano 4 (C) www.peano-framework.org " << std::endl;
    std::string buildInfo = "build: ";

    buildInfo += std::to_string(Dimensions);
    buildInfo += "d";

    #ifdef Parallel
    buildInfo += ", with mpi (";
    buildInfo += std::to_string( tarch::mpi::Rank::getInstance().getNumberOfRanks() );
    buildInfo += " ranks)";
    #else
    buildInfo += ", no mpi";
    #endif

    #if defined(SharedOMP) and defined(LayeredMultitaskingRuntime)
    buildInfo += ", omp(layered) (";
    buildInfo += std::to_string( tarch::multicore::Core::getInstance().getNumberOfThreads() );
    buildInfo += " threads)";
    #elif defined(SharedOMP) and !defined(LayeredMultitaskingRuntime)
    buildInfo += ", omp (";
    buildInfo += std::to_string( tarch::multicore::Core::getInstance().getNumberOfThreads() );
    buildInfo += " threads)";
    #elif defined(SharedTBB)
    buildInfo += ", tbb (";
    buildInfo += std::to_string( tarch::multicore::Core::getInstance().getNumberOfThreads() );
    buildInfo += " threads)";
    #elif defined(SharedCPP)
    buildInfo += ", C++ threading (";
    buildInfo += std::to_string( tarch::multicore::Core::getInstance().getNumberOfThreads() );
    buildInfo += " threads)";
    #else
    buildInfo += ", no threading";
    #endif

    #if PeanoDebug>0
    buildInfo += ", debug level=" + std::to_string(PeanoDebug);
    #endif

    std::cout << buildInfo << std::endl;
  }
  wrote = true;
}


void peano4::fillLookupTables() {
/*
  Had in in Globals.cpp

  setupLookupTableForTwoPowI();
  setupLookupTableForThreePowI();
  setupLookupTableForFourPowI();
  setupLookupTableForDPowI();
*/

  peano4::utils::setupLookupTableForDLinearised();
  peano4::utils::setupLookupTableForDDelinearised();
}


int peano4::initParallelEnvironment(int* argc, char*** argv) {
  int result = 0;
  #ifdef Parallel
  if ( tarch::mpi::Rank::getInstance().init(argc,argv) ) {
    peano4::parallel::Node::initMPIDatatypes();

  	clock_t timeout = 60;
    tarch::mpi::Rank::getInstance().setTimeOutWarning(timeout/4);
    tarch::mpi::Rank::getInstance().setDeadlockTimeOut(timeout);
  }
  else {
    result = -2;
  }
  #endif

  #ifdef SharedMemoryParallelisation
  if ( tarch::multicore::Core::getInstance().isInitialised() ) {
  }
  else {
    result = -3;
  }
  #endif

  writeCopyrightMessage();

  return result;
}


void peano4::shutdownParallelEnvironment() {
  writeBibTeXMessage();

  tarch::multicore::Core::getInstance().shutdown();
  peano4::parallel::Node::shutdownMPIDatatypes();
  tarch::mpi::Rank::getInstance().shutdown();
}


void peano4::initSingletons(
  const tarch::la::Vector<Dimensions,double>&  offset,
  const tarch::la::Vector<Dimensions,double>&  width,
  const std::bitset<Dimensions>&               periodicBC
) {
  tarch::services::ServiceRepository::getInstance().init();

  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().init();

  peano4::parallel::Node::getInstance().init();

  peano4::parallel::SpacetreeSet::getInstance().init(
    offset,
    width,
    periodicBC
  );

  writeCopyrightMessage();
}


void peano4::shutdownSingletons() {
  writeBibTeXMessage();

  peano4::parallel::Node::getInstance().shutdown();

  peano4::parallel::SpacetreeSet::getInstance().shutdown();

  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().shutdown();

  tarch::services::ServiceRepository::getInstance().shutdown();
}
