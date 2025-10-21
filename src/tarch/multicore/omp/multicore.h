// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#if defined(SharedOMP)


namespace tarch {
  namespace multicore {
    namespace omp {
      /**
       * Find out major OpenMP version
       *
       * Returns an integer which is the major OpenMP version. At the moment,
       * this is a value between 1 and 6. But the routine might also 0 if it
       * is not sure about the version.
       *
       * ## Transparent tasks
       *
       * In OpenMP, dependencies of tasks can, by default, only be used to specify
       * dependencies between children and descendants of the same task. That is,
       * if we have to siblings A and B and both spawn complex DAGs, these DAGs
       * cannot have dependencies. It seems to work sometimes, but notably not if
       * B kicks off way after A terminates (e.g. as they sit in different
       * traversals).
       *
       * In OpenMP 6, this will change as it introduces transparent task
       * dependencies. Lacking transparency in most runtimes, we have to track
       * if things are messed up manually.
       *
       * @return Major OpenMP version counter
       */
      constexpr int majorOpenMPVersion() {
        #if _OPENMP==199810
        return 1;
        #elif _OPENMP==200203 or _OPENMP==200505
        return 2; // 2.0 and 2.5
        #elif _OPENMP==200805 or _OPENMP==201107
        return 3; // 3.0 or 3.1
        #elif _OPENMP==201307 or _OPENMP==201511
        return 4; // 4.0 or 4.5
        #elif _OPENMP==201811 or _OPENMP==202011 or _OPENMP==202111
        return 5; // 5.0 or 5.1 or 5.2
        #elif _OPENMP==202411
        return 6;
        #else
        return 0;
        #endif
      }
    }
  }
}


#endif
