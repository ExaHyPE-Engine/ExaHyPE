// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::fv::tests {
  struct CellLabel;
  struct FaceLabel;

  template <int Values>
  struct Patch;

  template <typename Element>
  INLINE_METHOD peano4::dataview::FaceEnumerator<Element> enumerateFaces(
    std::array<Element, peano4::FacesPerCell>& faces
  );
} // namespace exahype2::fv::tests

//! Stands in for the generated cell label.
struct exahype2::fv::tests::CellLabel final {
  [[nodiscard]] INLINE_METHOD double getTimeStamp() const;
  INLINE_METHOD void                 setTimeStamp(double value);
  [[nodiscard]] INLINE_METHOD double getTimeStepSize() const;
  INLINE_METHOD void                 setTimeStepSize(double value);
  [[nodiscard]] INLINE_METHOD bool   getHasUpdated() const;
  INLINE_METHOD void                 setHasUpdated(bool value);
  [[nodiscard]] INLINE_METHOD bool   getRequiresCriterionEvaluation() const;
  INLINE_METHOD void                 setRequiresCriterionEvaluation(bool value);

private:
  double _timeStamp{0.0};
  double _timeStepSize{0.0};
  bool   _hasUpdated{false};
  bool   _requiresCriterionEvaluation{false};
};

/**
 * Stands in for the generated face label. Half zero is the left side of the face and half one
 * the right side.
 */
struct exahype2::fv::tests::FaceLabel final {
  [[nodiscard]] INLINE_METHOD bool getBoundary() const;
  INLINE_METHOD void               setBoundary(bool value);
  [[nodiscard]] INLINE_METHOD bool getIsHanging() const;
  INLINE_METHOD void               setIsHanging(bool value);

  [[nodiscard]] INLINE_METHOD bool getAboveHanging(int half) const;
  INLINE_METHOD void               setAboveHanging(int half, bool value);
  [[nodiscard]] INLINE_METHOD bool getUpdated(int half) const;
  INLINE_METHOD void               setUpdated(int half, bool value);
  [[nodiscard]] INLINE_METHOD bool getRequiresDataTransfer(int half) const;
  INLINE_METHOD void               setRequiresDataTransfer(int half, bool value);

  [[nodiscard]] INLINE_METHOD double getUpdatedTimeStamp(int half) const;
  INLINE_METHOD void                 setUpdatedTimeStamp(int half, double value);
  [[nodiscard]] INLINE_METHOD double getNewTimeStamp(int half) const;
  INLINE_METHOD void                 setNewTimeStamp(int half, double value);
  [[nodiscard]] INLINE_METHOD double getOldTimeStamp(int half) const;
  INLINE_METHOD void                 setOldTimeStamp(int half, double value);

  [[nodiscard]] INLINE_METHOD std::string toString() const;

private:
  bool                  _boundary{false};
  bool                  _isHanging{false};
  std::array<bool, 2>   _aboveHanging{false, false};
  std::array<bool, 2>   _updated{false, false};
  std::array<bool, 2>   _requiresDataTransfer{false, false};
  std::array<double, 2> _updatedTimeStamp{0.0, 0.0};
  std::array<double, 2> _newTimeStamp{0.0, 0.0};
  std::array<double, 2> _oldTimeStamp{0.0, 0.0};
};

//! Holds the values of one cell patch or one face overlap.
template <int Values>
struct exahype2::fv::tests::Patch final {
  static constexpr int Cardinality = Values;

  [[nodiscard]] INLINE_METHOD double*       data();
  [[nodiscard]] INLINE_METHOD const double* data() const;

  INLINE_METHOD void fill(double value);

private:
  std::array<double, Values> _values{};
};

#include "exahype2/fv/tests/ActionSetTestData.cpph"
