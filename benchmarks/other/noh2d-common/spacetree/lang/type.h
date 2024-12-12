#pragma once

#include <cstdint>
#include <optional>

using i8 = std::int8_t;
using u8 = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

// https://stackoverflow.com/a/63454953/5769882
struct Any {
    template <typename T>
    operator T&();

    template <typename T>
    operator T&&();
};

template<typename T> struct ptr {
    using value = T*;
};

using void_ptr = ptr<void>::value;

// https://stackoverflow.com/a/3221914/5769882
#define STRCAT_INNER(a, b) a##b
#define STRCAT(a, b) STRCAT_INNER(a, b)

template<typename T>
concept Dimensionality = requires(T t) {
    { T::N } -> std::same_as<const u32 &>;
};

template<u32 dims>
struct ND {
    static const constexpr u32 N = dims;
};

using _1D = ND<1>;
static_assert(Dimensionality<_1D>);

using _2D = ND<2>;
static_assert(Dimensionality<_2D>);

using _3D = ND<3>;
static_assert(Dimensionality<_3D>);

template<typename T>
concept Precision = std::floating_point<T>;

template<typename T>
concept NotVoid = !std::is_same_v<T, void>;

template<typename T>
concept Optional = std::same_as<T, std::optional<typename T::value_type>>;

template<typename T, typename V>
concept OptionalValue = std::same_as<V, std::optional<typename T::value_type>>;
