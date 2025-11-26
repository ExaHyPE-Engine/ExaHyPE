// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <map>
#include <string>

namespace peano4::grid {
  /**
   * Instruction to split
   *
   * An instruction to split always holds the number of splits that we should
   * fork off, and it also contains an instruction how we want the tree to
   * split the data off. We can either do the bottom up scheme, where we try
   * to find exactly the right number of fine grid cells on the finest level
   * and then assign refined cells to remote trees where it is a fit. The
   * realisation of these splitting variants is discussed in peano4::grid::Spacetree::isCellBottomUpSplitCandidate(),
   * ::grid::Spacetree::isCellTopDownSplitCandidate()
   * and peano4::grid::Spacetree::splitOrJoinCell(). The underlying algorithmic
   * considerations are documented in @ref peano_domain_decomposition "the domain decomposition overview".
   *
   * I have no constructor for this struct, but you can easily create one via
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   * SplitInstruction{24,SplitInstruction::Mode::AggressiveTopDown}
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * using the brace initialisation of C++.
   */
  struct SplitInstruction {
    enum class Mode { AggressiveTopDown, BottomUp };

    int  numberOfFineGridCells{-1};
    Mode mode{Mode::BottomUp};

    static std::string toString(Mode mode);
    std::string        toString() const;
  };

  using SplitSpecification = std::map<int, SplitInstruction>;
} // namespace peano4::grid

std::ostream& operator<<(std::ostream& out, const peano4::grid::SplitInstruction& instruction);
std::ostream& operator<<(std::ostream& out, const peano4::grid::SplitInstruction::Mode& mode);
