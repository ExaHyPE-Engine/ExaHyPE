// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  class RunControl;
} // namespace runner

/**
 * Run control of the parallel program. The global master chooses the next
 * program step, and continueToRun() hands it to every rank once per sweep,
 * carrying any non-critical assertion along in the same reduction.
 */
class runner::RunControl final {
public:
  /** Program step that ends the run on every rank. */
  static constexpr int Terminate = -2;

  /** Program step while no step has been chosen yet. */
  static constexpr int UndefProgramStep = -1;

  RunControl(const RunControl&)            = delete;
  RunControl(RunControl&&)                 = delete;
  RunControl& operator=(const RunControl&) = delete;
  RunControl& operator=(RunControl&&)      = delete;
  ~RunControl()                            = default;

  static RunControl& getInstance();

  bool continueToRun();

  void setNextProgramStep(int programStep);

  int getCurrentProgramStep() const;

private:
  RunControl() = default;

  static RunControl _singleton;

  int _currentProgramStep{UndefProgramStep};
};
