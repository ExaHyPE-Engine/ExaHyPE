// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace tarch::la {
  template <int Size, typename Scalar>
  struct SymmetricMatrix;

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD SymmetricMatrix<Size, Scalar> operator+(
    const SymmetricMatrix<Size, Scalar>& left,
    const SymmetricMatrix<Size, Scalar>& right
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD SymmetricMatrix<Size, Scalar> operator-(
    const SymmetricMatrix<Size, Scalar>& left,
    const SymmetricMatrix<Size, Scalar>& right
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD SymmetricMatrix<Size, Scalar> operator*(
    const SymmetricMatrix<Size, Scalar>& matrix,
    const Scalar&                        scalar
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD SymmetricMatrix<Size, Scalar> operator*(
    const Scalar&                        scalar,
    const SymmetricMatrix<Size, Scalar>& matrix
  );

  template <int Size, typename Scalar>
  INLINE_METHOD GPU_CALLABLE_METHOD Vector<Size, Scalar> operator*(
    const SymmetricMatrix<Size, Scalar>& matrix,
    const Vector<Size, Scalar>&          vector
  );

  template <int Size, typename Scalar>
  INLINE_METHOD std::ostream& operator<<(std::ostream& os, const SymmetricMatrix<Size, Scalar>& matrix);
} // namespace tarch::la

/**
 * Stores the diagonal and then the upper triangle row by row of a symmetric matrix. A 3D matrix
 * stores the entries xx, yy, zz, xy, xz and yz.
 */
template <int Size, typename Scalar>
struct tarch::la::SymmetricMatrix final {
public:
  static constexpr int NumberOfEntries = Size * (Size + 1) / 2;

  SymmetricMatrix() = default;
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr SymmetricMatrix(std::initializer_list<Scalar> values);

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar& operator()(int rowIndex, int columnIndex);
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar& operator()(int rowIndex, int columnIndex) const;

  INLINE_METHOD GPU_CALLABLE_METHOD Scalar* getData();
  INLINE_METHOD GPU_CALLABLE_METHOD const Scalar* getData() const;

  INLINE_METHOD std::string toString() const;

private:
  Scalar _values[NumberOfEntries]{};

  INLINE_METHOD GPU_CALLABLE_METHOD static constexpr int getEntryOf(int rowIndex, int columnIndex);
  INLINE_METHOD GPU_CALLABLE_METHOD static constexpr int computeEntryOf(int rowIndex, int columnIndex);
  INLINE_METHOD GPU_CALLABLE_METHOD static constexpr std::uint64_t getPackedEntries();
};
TARGET_DECLARE_END

#include "tarch/la/SymmetricMatrix.cpph"
