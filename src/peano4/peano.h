// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_H_
#define _PEANO4_H_

#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"

/**
 *
 * @mainpage Peano 4
 *
 * Welcome to the source code documentation of Peano 4.
 *
 * @image html peano4/architecture-layers.png
 *
 * @section Documentation
 *
 * The high-level documentation of Peano 4 is contained within a guidebook. The
 * guidebook is written in TeX and can be downloaded from the Peano webpage at
 * <a href="www.peano-framework.org">www.peano-framework.org</a>. It is automatically
 * updated around once a week. If you want to access the latest version, you have
 * to build the pdf yourself. Change into the cookbook directory of your checkout
 * (parallel to src) and run pdflatex. Our guidebook also contains extensive
 * build instructions.
 *
 *
 * Peano is shipped with a couple of low-level demonstrators. One of them is a mere
 * run of all of the unit tests. To run them, switch into src/examples/unittests.
 * Unit tests are held within subdirectories of the respective class/function that
 * they test. The way they use a function is some good documentation complementing
 * what we write into the documentation.
 *
 *
 * @section Rationale
 *
 * We follow the convention that the header files only hold the declarations, while
 * the cpp files hold the actual implementations. We kind of gave up on this convention
 * in few particular places for speed, i.e. whenever a compiler should do inlining,
 * i.e. remove function calls, but doesn't. If we found such places and suffered
 * significantly in terms of performance, then we moved the implementation into the
 * headers. We also use header-based implementations for template (the header
 * implementation files then have the extension .cpph), but as soon as compilers do
 * widely support templates that are not headers or we have moved functions over to
 * the new C++ auto keyword, we'll move all into the implementation files.
 *
 * I decided a while ago that all documentation should be in the headers if possible.
 * There's two reasons for that:
 *
 * - Most documentation focuses on what a function does not how it does it. Therefore,
 *   the docu belongs to the declaration.
 * - If an implementation is so complicated or long that you need documentation within
 *  the source code to understand it, then the function needs to be simplified or split up.
 *
 * These are only rules of thumb.
 *
 *
 * @section Syntax Conventions
 *
 * We mainly follow mainstream Java syntax conventions in Peano for historic reasons and
 * as this is a real OO/C++ project. Some important principles are:
 *
 * - One file per class
 * - Consistent naming conventions
 * - Long, self-explaining names (no acronyms)
 * - One subdirectory per namespace
 *
 */


/**
 * @dir peano
 *
 * Peano's core layer. It sits on top of the technical architecture.
 */
namespace peano4 {
  void writeCopyrightMessage();

  /**
   * Fill Lookup Tables
   *
   * Fill all the lookup tables used within the application. As lookup
   * tables are used by many operations, I suggest to call this operation
   * as soon as possible.
   *
   * There shall no error occur in this operation. Thus, it does not return
   * any code.
   */
  void fillLookupTables();

  /**
   * Init Parallel Environment
   *
   * Inits the parallel environment. If the parallel mode is not set, the
   * operation detoriates to nop. The function returns 0 if everything
   * is o.k., it returns -2 otherwise. Please call this operation before
   * you call any other operation that could result in an error. I suggest
   * to call it right after fillLookupTables().
   *
   * Please note that Peano 4 considers both shared memory and distributed
   * memory to be a parallel environment.
   *
   * init might change the variables passed. If you want to parse the
   * command line arguments, use the values returned. If you use the
   * arguments directly without calling initParallelEnvironment() they
   * might contain MPI values not important to the program.
   *
   * <h2> Usage/implementation details </h2>
   *
   * You may not use the trace macros before this operation has invoked the init
   * operation. Otherwise, the getRank() assertion fails, as the node has not
   * been configured correctly.
   *
   * Invoke with an address operator before that.
   * <pre>
  peano4::initParallelEnvironment(&argc,&argv);
     </pre>
   * This has to be done as one of the very first things, i.e. before you init
   * the logging, or run tests, or ...
   */
  int initParallelEnvironment(int* argc, char*** argv);

  /**
   * Shutdown all the parallel environment, i.e. free all MPI datatypes and
   * close down MPI. This also turns off the shared memory environment.
   * Before this happens, you have to shutdown the node such that everybody
   * knows that we are going down. So you have to call Node::shutdown()
   * before you trigger this operation. This is your responsibility.
   *
   * The routine first adds a barrier. This barrier is necessary. If the very last
   * activity of all ranks is for example to plot stuff, they typically use
   * global semaphores as well. To make these semaphores work, we still require
   * that all nodes call receiveDanglingMessages(). It is only after everyone
   * has done their dump, that we can shut down the shared memory system. This is the
   * reason the barrier has to come after the node's shutdown and then has to
   * be a Peano 4 barrier which still invokes receiveDanglingMessages() on all
   * services.
   *
   * Once all shared memory tasks have terminated, we free the MPI datatypes.
   *
   * Eventually, we shut down the MPI rank.
   *
   * Once this routine has terminated, do not add any barrier() anymore!
   *
   * @see peano4::parallel::Node::shutdown()
   */
  void shutdownParallelEnvironment();

  /**
   * Fire up all the singletons.
   *
   *
   * Singletons that I don't touch are:
   *
   * - tarch::mpi::Rank, as the rank is not a service and is handled
   *   separately by the initParallelEnvironment().
   */
  void initSingletons(
    const tarch::la::Vector<Dimensions,double>&  offset,
    const tarch::la::Vector<Dimensions,double>&  width,
    const std::bitset<Dimensions>&               periodicBC = 0
  );

  /**
   * The very first thing I have to do is to shut down Node. This
   * shutdown will tell all the other ranks to go down as well.
   */
  void shutdownSingletons();
}


#endif


