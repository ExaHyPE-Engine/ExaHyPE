// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_PARALLEL_H_
#define _PEANO4_PARALLEL_H_


namespace peano4 {
  /**
   * \namespace parallel
   *
   * The parallel namespace is Peano's core abstracts from both MPI and
   * multicore parallelisation.
   *
   *
   * \section Recreate all DaStGen objects
   *
   * <pre>
java -jar ~/git/DaStGen/DaStGen.jar --plugin PeanoSnippetGenerator --naming Peano4NameTranslator TreeEntry.def  .
java -jar ~/git/DaStGen/DaStGen.jar --plugin PeanoSnippetGenerator --naming Peano4NameTranslator StartTraversalMessage.def  .
java -jar ~/git/DaStGen/DaStGen.jar --plugin PeanoSnippetGenerator --naming Peano4NameTranslator TreeManagementMessage.def  .
    </pre>
   *
   */
  namespace parallel {
  }
}

#endif
