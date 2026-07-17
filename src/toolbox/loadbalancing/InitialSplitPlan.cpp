// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InitialSplitPlan.h"

std::vector<int>
toolbox::loadbalancing::InitialSplitPlan::balancedPieceLoads(const int totalLoad, const int numberOfPieces) {
  const auto       cellsPerPiece = totalLoad / numberOfPieces;
  const auto       remainder     = totalLoad % numberOfPieces;
  std::vector<int> pieceLoads(numberOfPieces, cellsPerPiece);
  for (int pieceIndex = 0; pieceIndex < remainder; ++pieceIndex) {
    ++pieceLoads[pieceIndex];
  }
  return pieceLoads;
}

/** A completed uniform grid cannot expose additional cells for initial trees. */
toolbox::loadbalancing::InitialSplitPlan::Decomposition toolbox::loadbalancing::InitialSplitPlan::classifyDecomposition(
  const int           configuredNumberOfTrees,
  const std::uint64_t availableNumberOfCells,
  const bool          uniformGridComplete
) {
  return uniformGridComplete and static_cast<std::uint64_t>(configuredNumberOfTrees) > availableNumberOfCells
    ? toolbox::loadbalancing::InitialSplitPlan::Decomposition::InsufficientCells
    : toolbox::loadbalancing::InitialSplitPlan::Decomposition::Feasible;
}

/** A partial rank distribution cannot continue without introducing unbounded rank imbalance. */
toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution
toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(
  const int numberOfRanks,
  const int numberOfPopulatedRanks
) {
  if (numberOfRanks <= 1 or numberOfPopulatedRanks >= numberOfRanks) {
    return toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Complete;
  }
  return numberOfPopulatedRanks > 1 ? toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Partial
                                    : toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::SoleOwner;
}

/** Retaining one balanced piece prevents the source tree from becoming empty. */
toolbox::loadbalancing::InitialSplitPlan
toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(const int sourceLoad, const std::vector<int>& targetRanks) {
  toolbox::loadbalancing::InitialSplitPlan result;
  if (sourceLoad <= 1 or targetRanks.empty()) {
    return result;
  }

  const auto numberOfChildren = std::min(static_cast<int>(targetRanks.size()), sourceLoad - 1);
  const auto pieceLoads =
    toolbox::loadbalancing::InitialSplitPlan::balancedPieceLoads(sourceLoad, numberOfChildren + 1);
  result.requests.reserve(numberOfChildren);
  for (int childIndex = 0; childIndex < numberOfChildren; ++childIndex) {
    result.requests.push_back({pieceLoads[childIndex], targetRanks[childIndex]});
  }
  return result;
}

/** Partitioning rank loads before tree loads bounds the rank imbalance to one cell. */
toolbox::loadbalancing::InitialSplitPlan toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(
  const int sourceLoad,
  const int sourceRank,
  const int numberOfRanks,
  const int configuredNumberOfTrees
) {
  toolbox::loadbalancing::InitialSplitPlan result;
  const auto                               numberOfPieces = std::min(sourceLoad, configuredNumberOfTrees);
  if (numberOfPieces <= 1) {
    return result;
  }

  result.requests.reserve(numberOfPieces - 1);
  const auto numberOfPopulatedRanks = std::min(numberOfRanks, numberOfPieces);
  const auto rankLoads =
    toolbox::loadbalancing::InitialSplitPlan::balancedPieceLoads(sourceLoad, numberOfPopulatedRanks);
  const auto       basePiecesPerRank   = numberOfPieces / numberOfPopulatedRanks;
  const auto       ranksWithExtraPiece = numberOfPieces % numberOfPopulatedRanks;
  std::vector<int> populatedRanks;
  populatedRanks.reserve(numberOfPopulatedRanks);
  populatedRanks.push_back(sourceRank);
  int numberOfSelectedRanks = 1;
  for (int targetRank = 0; targetRank < numberOfRanks and numberOfSelectedRanks < numberOfPopulatedRanks;
       ++targetRank) {
    if (targetRank != sourceRank) {
      populatedRanks.push_back(targetRank);
      ++numberOfSelectedRanks;
    }
  }
  std::sort(populatedRanks.begin(), populatedRanks.end());

  for (int rankIndex = 0; rankIndex < numberOfPopulatedRanks; ++rankIndex) {
    const auto targetRank   = populatedRanks[rankIndex];
    const auto piecesOnRank = basePiecesPerRank + (rankIndex < ranksWithExtraPiece ? 1 : 0);
    auto pieceLoads = toolbox::loadbalancing::InitialSplitPlan::balancedPieceLoads(rankLoads[rankIndex], piecesOnRank);
    if (targetRank == sourceRank) {
      pieceLoads.pop_back();
    }
    // Request order determines the spatial segments assigned to monotonically allocated worker IDs.
    std::reverse(pieceLoads.begin(), pieceLoads.end());
    for (const auto pieceLoad : pieceLoads) {
      result.requests.push_back({pieceLoad, targetRank});
    }
  }

  return result;
}
