// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SplitInstruction.h"

#include <sstream>

std::string peano4::grid::SplitInstruction::toString(SplitInstruction::Mode mode) {
  switch (mode) {
  case SplitInstruction::Mode::AggressiveTopDown:
    return "AggressiveTopDown";
  case SplitInstruction::Mode::BottomUp:
    return "BottomUp";
  }
  return "<undef>";
}

std::string peano4::grid::SplitInstruction::toString() const {
  std::ostringstream msg;
  msg << "(" << toString(mode) << ",#Cells=" << numberOfFineGridCells << ")";
  return msg.str();
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::SplitInstruction& instruction) {
  out << instruction.toString();
  return out;
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::SplitInstruction::Mode& mode) {
  out << peano4::grid::SplitInstruction::toString(mode);
  return out;
}
