// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::grid {
  /**
   * Describes a split request whose requested cell count has to be satisfied.
   *
   * Bottom-up splitting assigns exactly the requested number of unrefined
   * fine-grid cells to the new tree. If grid topology prevents completion,
   * the traversal terminates instead of accepting a partial decomposition.
   * Aggressive top-down splitting moves whole subtrees and can therefore
   * exceed the requested count.
   *
   * Spacetree::splitOrJoinCellBottomUp() and Spacetree::splitCellTopDown()
   * implement these modes. SpacetreeHelper::isSpacetreeNodeSplitCandidate()
   * defines cell eligibility; @ref page_peano_domain_decomposition "the domain
   * decomposition overview" explains how requests shape a partition.
   */
  struct SplitInstruction final {
    enum class Mode { AggressiveTopDown, BottomUp };

    int  numberOfFineGridCells{-1};
    Mode mode{Mode::BottomUp};

    static std::string toString(Mode mode);
    std::string        toString() const;
  };

  using SplitSpecification = std::map<int, SplitInstruction>;
} // namespace peano4::grid

std::ostream& operator<<(std::ostream& outputStream, const peano4::grid::SplitInstruction& instruction);
std::ostream& operator<<(std::ostream& outputStream, const peano4::grid::SplitInstruction::Mode& mode);
