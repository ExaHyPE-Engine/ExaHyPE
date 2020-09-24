// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_USER_INTERFACE_H_
#define _EXAHYPE2_USER_INTERFACE_H_



namespace exahype2 {
  bool parseCommandLineArguments(int argc, char** argv);

  void printUsage(char** argv);

  void setDefaultLogStatements();
}


#endif

