// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchUtils.h"

/** Returns the isotropic volume width along the first coordinate axis. */
double exahype2::fv::getVolumeLength(const tarch::la::Vector<2, double>& h, int numberOfVolumesPerAxisInPatch) {
  return h(0) / numberOfVolumesPerAxisInPatch;
}

/** Returns the isotropic volume width along the first coordinate axis. */
double exahype2::fv::getVolumeLength(const tarch::la::Vector<3, double>& h, int numberOfVolumesPerAxisInPatch) {
  assertion(numberOfVolumesPerAxisInPatch >= 1, h, numberOfVolumesPerAxisInPatch);
  return h(0) / numberOfVolumesPerAxisInPatch;
}

/** Returns a diagnostic, non-round-trip representation of one volume. */
std::string exahype2::fv::plotVolume(const double* NOALIAS Q, int unknowns) {
  std::string description = "(" + std::to_string(Q[0]);
  for (int quantity = 1; quantity < unknowns; quantity++) {
    description += "," + std::to_string(Q[quantity]);
  }
  description += ")";
  return description;
}

namespace {
  /**
   * Formats one diagnostic patch entry and inserts its separator.
   *
   * State is thread-local, so concurrent plot calls do not race. The result is
   * not a lossless serialisation: std::to_string limits precision and release
   * builds compress consecutive zero entries.
   */
  template <typename T>
  std::string prettyFormat(T value, bool isFirst) {
    [[maybe_unused]] thread_local static int previousZeroDataEntries = 0;

    if (tarch::la::equals(double(value), 0.0, 1e-8) and isFirst) {
      previousZeroDataEntries = 1;
      return "0";
    } else if (isFirst) {
      previousZeroDataEntries = 0;
      return std::to_string(value);
    }
#if PEANO_DEBUG
    else {
      return "," + std::to_string(value);
    }
#else
    else if (not tarch::la::equals(double(value), 0.0, 1e-8)) {
      previousZeroDataEntries = 0;
      return "," + std::to_string(value);
    } else if (previousZeroDataEntries == 0) {
      previousZeroDataEntries = 1;
      return ",0";
    } else if (previousZeroDataEntries == 1) {
      previousZeroDataEntries = 2;
      return ",...";
    } else {
      return "";
    }
#endif
  }
} // namespace

/**
 * Returns a diagnostic representation of an array-of-structures patch.
 *
 * The routine is thread-safe when Q is not mutated concurrently. It is not a
 * lossless serialisation; prettyPrint controls line breaks, not precision.
 */
template <typename T>
std::string exahype2::fv::plotPatch(
  const T* NOALIAS Q,
  int              unknowns,
  int              auxiliaryVariables,
  int              numberOfVolumesPerAxisInPatch,
  int              haloSize,
  bool             prettyPrint
) {
  const int patchSize = numberOfVolumesPerAxisInPatch + 2 * haloSize;

  std::ostringstream description;

  if (prettyPrint) {
    description << std::endl;
  }

#if DIMENSIONS == 2
  for (int y = 0; y < patchSize; y++) {
    for (int x = 0; x < patchSize; x++) {
      const int volumeOffset = (y * patchSize + x) * (unknowns + auxiliaryVariables);

      const bool isDiagonal = (x < haloSize and y < haloSize)
        or (x >= numberOfVolumesPerAxisInPatch + haloSize and y < haloSize)
        or (x < haloSize and y >= numberOfVolumesPerAxisInPatch + haloSize)
        or (x >= numberOfVolumesPerAxisInPatch + haloSize and y >= numberOfVolumesPerAxisInPatch + haloSize);

      description << "(";
      for (int quantity = 0; quantity < unknowns + auxiliaryVariables; quantity++) {
        const int entry = volumeOffset + quantity;

        if (not isDiagonal) {
          description << prettyFormat(Q[entry], quantity == 0);
        } else {
          description << "x";
        }
      }
      description << ")";
    }
    description << std::endl;
  }
#else
  peano4::dForEach(patchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& patchIndex) {
    const int volumeOffset = peano4::dLinearised(patchIndex, patchSize) * (unknowns + auxiliaryVariables);

    const bool isDiagonal = (tarch::la::count(patchIndex, 0) + tarch::la::count(patchIndex, patchSize - 1)) > 1;

    description << "(";
    for (int quantity = 0; quantity < unknowns + auxiliaryVariables; quantity++) {
      const int entry = volumeOffset + quantity;

      if (quantity != 0) {
        description << ",";
      }

      if (haloSize == 0 or not isDiagonal) {
        description << Q[entry];
      } else {
        description << "x";
      }
    }
    description << ")";
  });
#endif

  if (prettyPrint) {
    description << std::endl;
  }

  return description.str();
}

template std::string exahype2::fv::plotPatch<float>(const float* NOALIAS, int, int, int, int, bool);
template std::string exahype2::fv::plotPatch<double>(const double* NOALIAS, int, int, int, int, bool);

/**
 * Returns a diagnostic representation of both layers of a face overlap.
 *
 * The routine is thread-safe when Q is not mutated concurrently and is not a
 * lossless serialisation. Three-dimensional pretty printing is unavailable;
 * the compact representation supports both dimensions.
 */
template <typename T>
std::string exahype2::fv::plotPatchOverlap(
  const T* NOALIAS Q,
  int              unknowns,
  int              auxiliaryVariables,
  int              numberOfVolumesPerAxisInPatch,
  int              haloSize,
  int              normal,
  bool             prettyPrint
) {
  assertion(normal >= 0);
  assertion(normal < DIMENSIONS);

  std::ostringstream description;
  if (prettyPrint) {
    description << std::endl;

#if DIMENSIONS == 2
    if (normal == 0) {
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++) {
        for (int x = 0; x < 2 * haloSize; x++) {
          const int volumeOffset = (y * numberOfVolumesPerAxisInPatch + x) * (unknowns + auxiliaryVariables);
          description << "(";
          for (int quantity = 0; quantity < unknowns + auxiliaryVariables; quantity++) {
            const int entry = volumeOffset + quantity;
            description << prettyFormat(Q[entry], quantity == 0);
          }
          description << ")";
          if (x == haloSize - 1) {
            description << " | ";
          }
        }
        description << std::endl;
      }
    } else {
      for (int y = 0; y < 2 * haloSize; y++) {
        for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
          const int volumeOffset = (y * numberOfVolumesPerAxisInPatch + x) * (unknowns + auxiliaryVariables);
          description << "(";
          for (int quantity = 0; quantity < unknowns + auxiliaryVariables; quantity++) {
            const int entry = volumeOffset + quantity;
            description << prettyFormat(Q[entry], quantity == 0);
          }
          description << ")";
        }
        description << std::endl;

        if (y == haloSize - 1) {
          for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
            description << " --- ";
          }
        }
      }
    }
#else
    description << "<3D pretty printing is unavailable>";
#endif
    description << std::endl;
  } else {
    int valueOffset = 0;
    for (int overlapIndex = 0; overlapIndex < 2 * haloSize; overlapIndex++) {
#if DIMENSIONS == 3
      for (int firstTangentialIndex = 0; firstTangentialIndex < numberOfVolumesPerAxisInPatch; firstTangentialIndex++) {
#endif
        for (int secondTangentialIndex = 0; secondTangentialIndex < numberOfVolumesPerAxisInPatch;
             secondTangentialIndex++) {
          if (secondTangentialIndex == 0) {
            description << "(";
          } else {
            description << ",(";
          }
          for (int quantity = 0; quantity < unknowns + auxiliaryVariables; quantity++) {
            description << prettyFormat(Q[valueOffset], quantity == 0);
            valueOffset++;
          }
          description << ")";
        }
#if DIMENSIONS == 3
      }
#endif
    }
  }

  return description.str();
}

template std::string exahype2::fv::plotPatchOverlap<float>(const float* NOALIAS, int, int, int, int, int, bool);
template std::string exahype2::fv::plotPatchOverlap<double>(const double* NOALIAS, int, int, int, int, int, bool);
