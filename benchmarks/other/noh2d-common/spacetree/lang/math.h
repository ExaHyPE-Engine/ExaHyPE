#pragma once

#include <concepts>

#include <lang/type.h>

template<typename T> requires std::is_integral_v<T>
constexpr T pow_constexpr(T val, T exp) {
    T res = 1;
    for (u32 i = 0; i < exp; i++) res *= val;
    return res;
}
