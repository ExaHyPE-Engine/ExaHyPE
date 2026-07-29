// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::parallel {
  class RunControl;
} // namespace peano4::parallel

/**
 * Run control of the parallel program. The global master chooses the next
 * program step, and continueToRun() hands it to every rank once per sweep,
 * carrying any non-critical assertion along in the same reduction.
 */
class peano4::parallel::RunControl final {
public:
  static constexpr int Terminate = -2;

  /**
   * Program step while no step has been chosen yet.
   */
  static constexpr int UndefProgramStep = -1;

  ~RunControl()                            = default;
  RunControl(const RunControl&)            = delete;
  RunControl(RunControl&&)                 = delete;
  RunControl& operator=(const RunControl&) = delete;
  RunControl& operator=(RunControl&&)      = delete;

  static RunControl& getInstance();

  bool continueToRun();

  /**
   * Choose the program step that continueToRun() distributes next. Only the
   * global master's choice reaches the other ranks.
   */
  void setNextProgramStep(const int number);

  int getCurrentProgramStep() const;

private:
  RunControl() = default;

  static RunControl _singleton;

  int _currentProgramStep{UndefProgramStep};
};
