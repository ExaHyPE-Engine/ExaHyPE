// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace tarch::plotter {
  template <typename T>
  class DefaultInitialisingAllocator;

  template <typename T>
  using MeshArray = std::vector<T, DefaultInitialisingAllocator<T>>;

  struct Mesh;
} // namespace tarch::plotter

/**
 * Leaves the new entries of a number type uninitialised in a resize().
 */
template <typename T>
class tarch::plotter::DefaultInitialisingAllocator : public std::allocator<T> {
public:
  DefaultInitialisingAllocator()                                                     = default;
  DefaultInitialisingAllocator(const DefaultInitialisingAllocator& other)            = default;
  DefaultInitialisingAllocator(DefaultInitialisingAllocator&& other)                 = default;
  DefaultInitialisingAllocator& operator=(const DefaultInitialisingAllocator& other) = default;
  DefaultInitialisingAllocator& operator=(DefaultInitialisingAllocator&& other)      = default;
  ~DefaultInitialisingAllocator()                                                    = default;

  template <typename U>
  INLINE_METHOD DefaultInitialisingAllocator(const DefaultInitialisingAllocator<U>& other) noexcept;

  template <typename U>
  INLINE_METHOD void construct(U* pointer) noexcept(std::is_nothrow_default_constructible_v<U>);

  template <typename U, typename... Arguments>
  INLINE_METHOD void construct(U* pointer, Arguments&&... arguments);
};

/**
 * Stores the points, the cells and the fields of the part of a plot that one rank writes. The array
 * offsets lists the end of every cell in connectivity.
 */
struct tarch::plotter::Mesh final {
  struct Field final {
    std::string       name{};
    int               components{1};
    MeshArray<double> values{};
  };

  MeshArray<double>       pointsXYZ{};
  MeshArray<std::int32_t> connectivity{};
  MeshArray<std::int32_t> offsets{};
  MeshArray<std::uint8_t> types{};
  std::vector<Field>      pointFields{};
  std::vector<Field>      cellFields{};
};

#include "tarch/plotter/Mesh.cpph"
