// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PatchUtils.h"

double exahype2::fv::getVolumeLength(const tarch::la::Vector<2, double>& h, int numberOfVolumesPerAxisInPatch) {
  return h(0) / numberOfVolumesPerAxisInPatch;
}

double exahype2::fv::getVolumeLength(const tarch::la::Vector<3, double>& h, int numberOfVolumesPerAxisInPatch) {
  assertion(numberOfVolumesPerAxisInPatch >= 1, h, numberOfVolumesPerAxisInPatch);
  return h(0) / numberOfVolumesPerAxisInPatch;
}

std::string exahype2::fv::plotVolume(const double* NOALIAS Q, int unknowns) {
  std::string result = "(" + std::to_string(Q[0]);
  for (int i = 1; i < unknowns; i++)
    result += "," + std::to_string(Q[i]);
  result += ")";
  return result;
}

namespace {
  /**
   * Helper for the patch plots. Truncates many entries and also
   * inserts the commas. The plotter is not thread-safe and not
   * particularly sophisticated.
   */
  template <typename T>
  std::string prettyFormat(T value, bool isFirst) {
    [[maybe_unused]] static int previousZeroDataEntries = 0;

    // first entry equals zero
    if (tarch::la::equals(double(value), 0.0, 1e-8) and isFirst) {
      previousZeroDataEntries = 1;
      return "0";
    }
    // first entry does not equal zero
    else if (isFirst) {
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
    } else if (tarch::la::equals(double(value), 0.0, 1e-8) and previousZeroDataEntries == 1) {
      previousZeroDataEntries += 1;
      return ",...";
    } else {
      assertion(false);
      return "<undef>";
    }
#endif
  }
} // namespace

template <typename T>
std::string exahype2::fv::plotPatch(
  const T* NOALIAS Q,
  int              unknowns,
  int              auxiliaryVariables,
  int              numberOfVolumesPerAxisInPatch,
  int              haloSize,
  bool             prettyPrint
) {
  const int PatchSize = numberOfVolumesPerAxisInPatch + 2 * haloSize;

  std::ostringstream result;

  if (prettyPrint)
    result << std::endl;

#if DIMENSIONS == 2
  for (int y = 0; y < PatchSize; y++) {
    for (int x = 0; x < PatchSize; x++) {
      int index = (y * PatchSize + x) * (unknowns + auxiliaryVariables);

      // It is a diagonal entry if this counter is bigger than 1. If it equals
      // 0, it is interior. If it equals 1, then this is a face-connected halo
      // entry.
      bool isDiagonal
        = (x < haloSize and y < haloSize) or (x >= numberOfVolumesPerAxisInPatch + haloSize and y < haloSize)
          or (x < haloSize and y >= numberOfVolumesPerAxisInPatch + haloSize)
          or (x >= numberOfVolumesPerAxisInPatch + haloSize and y >= numberOfVolumesPerAxisInPatch + haloSize);

      result << "(";
      for (int i = 0; i < unknowns + auxiliaryVariables; i++) {
        const int entry = index + i;

        if (not isDiagonal)
          result << prettyFormat(Q[entry], i == 0);
        else
          result << "x";
      }
      result << ")";
    }
    result << std::endl;
  }
#else
  dfor(k, PatchSize) {
    int index = peano4::utils::dLinearised(k, PatchSize) * (unknowns + auxiliaryVariables);

    // It is a diagonal entry if this counter is bigger than 1. If it equals
    // 0, it is interior. If it equals 1, then this is a face-connected halo
    // entry.
    bool isDiagonal = (tarch::la::count(k, 0) + tarch::la::count(k, PatchSize - 1)) > 1;

    result << "(";
    for (int i = 0; i < unknowns + auxiliaryVariables; i++) {
      const int entry = index + i;

      if (i != 0)
        result << ",";

      if (haloSize == 0 or not isDiagonal)
        result << Q[entry];
      else
        result << "x";
    }
    result << ")";
  }
#endif

  if (prettyPrint)
    result << std::endl;

  return result.str();
}

template std::string exahype2::fv::plotPatch<float>(const float* NOALIAS, int, int, int, int, bool);
template std::string exahype2::fv::plotPatch<double>(const double* NOALIAS, int, int, int, int, bool);

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

  std::ostringstream result;
  if (prettyPrint) {
    result << std::endl;

#if DIMENSIONS == 2
    if (normal == 0) {
      for (int y = 0; y < numberOfVolumesPerAxisInPatch; y++) {
        for (int x = 0; x < 2 * haloSize; x++) {
          int index = (y * numberOfVolumesPerAxisInPatch + x) * (unknowns + auxiliaryVariables);
          result << "(";
          for (int i = 0; i < unknowns + auxiliaryVariables; i++) {
            const int entry = index + i;
            result << prettyFormat(Q[entry], i == 0);
          }
          result << ")";
          if (x == haloSize - 1)
            result << " | ";
        }
        result << std::endl;
      }
    } else {
      for (int y = 0; y < 2 * haloSize; y++) {
        for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
          int index = (y * numberOfVolumesPerAxisInPatch + x) * (unknowns + auxiliaryVariables);
          result << "(";
          for (int i = 0; i < unknowns + auxiliaryVariables; i++) {
            const int entry = index + i;
            result << prettyFormat(Q[entry], i == 0);
          }
          result << ")";
        }
        result << std::endl;

        if (y == haloSize - 1) {
          for (int x = 0; x < numberOfVolumesPerAxisInPatch; x++) {
            result << " --- ";
          }
        }
      }
    }
#else
    result << "<not implemented yet>";
#endif
    result << std::endl;
  } else {
    int entry = 0;
    for (int overlap = 0; overlap < 2 * haloSize; overlap++)
#if DIMENSIONS == 3
      for (int i = 0; i < numberOfVolumesPerAxisInPatch; i++)
#endif
        for (int ii = 0; ii < numberOfVolumesPerAxisInPatch; ii++) {
          if (ii == 0) {
            result << "(";
          } else {
            result << ",(";
          }
          for (int iii = 0; iii < unknowns + auxiliaryVariables; iii++) {
            result << prettyFormat(Q[entry], iii == 0);
            entry++;
          }
          result << ")";
        }
  }

  return result.str();
}

template std::string exahype2::fv::plotPatchOverlap<float>(const float* NOALIAS, int, int, int, int, int, bool);
template std::string exahype2::fv::plotPatchOverlap<double>(const double* NOALIAS, int, int, int, int, int, bool);
