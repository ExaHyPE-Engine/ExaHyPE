// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



namespace swift2 {
  bool parseCommandLineArguments(int argc, char** argv);

  void printUsage(char** argv);

  void setDefaultLogStatements();

  namespace commandlinesettings {
    /**
     * Find out if user has enabled dynamic load balancing or
     * not. If this flag is false, the code should disable the load
     * balancing in the initialisation step.
     */
    bool enableDynamicLoadBalancing();
  }
}



