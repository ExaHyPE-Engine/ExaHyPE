// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2 {
  bool parseCommandLineArguments(int argc, char** argv);

  void printUsage(char** argv);

  void setDefaultLogStatements();
}
