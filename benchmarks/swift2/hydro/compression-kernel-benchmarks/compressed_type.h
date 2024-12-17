#pragma once

#include "type.h"

#define STRCAT_INNER(a, b) a##b
#define STRCAT(a, b) STRCAT_INNER(a, b)

template<typename X>
concept NUM = std::integral<X> || std::floating_point<X>;

template<typename T, u8 BITS = sizeof(T) * 8, u8 ALIGNMENT = alignof(T)> requires std::is_floating_point_v<T>
class alignas(ALIGNMENT) CompressedFloat {
    __attribute__((packed)) u64 data: BITS;

    T __get() const {
        union {
            u64 byte_data;
            T float_data;
        } U;

        auto dataLocal = this->data;

        U.byte_data = dataLocal << (sizeof(T) * 8 - BITS);
        auto float_data = U.float_data;
        return float_data;
    }

    void __set(T float_data) {
        union {
            u64 byte_data;
            T float_data;
        } U;

        U.float_data = float_data;
        this->data = U.byte_data >> (sizeof(T) * 8 - BITS);
    }

public:
    CompressedFloat() = default;

    CompressedFloat(const CompressedFloat &copy) = default;

    CompressedFloat(const T &value) {
        this->__set(value);
    }

    CompressedFloat &operator=(const CompressedFloat &other) = default;

    template<NUM X>
    CompressedFloat &operator=(const X &value) {
        this->__set((T) value);
        return *this;
    }

#define UNARY_PREFIX_OP(op)                         \
    CompressedFloat &operator op() {                \
        auto f = this->__get();                     \
        f op;                                       \
        this->__set(f);                             \
        return *this;                               \
    }                                               \

    UNARY_PREFIX_OP(++)
    UNARY_PREFIX_OP(--)

#define UNARY_POSTFIX_OP(op)                        \
    const CompressedFloat operator op(i32) {        \
        auto oldThis = CompressedFloat(*this);      \
        this->operator op();                        \
        return oldThis;                             \
    }                                               \

    UNARY_POSTFIX_OP(++)
    UNARY_POSTFIX_OP(--)

    CompressedFloat operator -() const {
        return CompressedFloat(-this->__get());
    }

#undef UNARY_POSTFIX_OP

#define ARITHMETIC_OP(op)                                                           \
    template<NUM X>                                                                 \
    CompressedFloat &operator STRCAT(op, =) (const X &scalar) {                     \
        this->__set(this->__get() op scalar);                                       \
        return *this;                                                               \
    }                                                                               \
                                                                                    \
    template<NUM X>                                                                 \
    CompressedFloat operator op (const X &scalar) const {                           \
        return CompressedFloat(*this) STRCAT(op, =) scalar;                         \
    }                                                                               \
                                                                                    \
    template<NUM X>                                                                 \
    friend X& operator STRCAT(op, =) (X &scalar, const CompressedFloat cf) {        \
        scalar STRCAT(op, =) cf.__get();                                            \
        return scalar;                                                              \
    }                                                                               \
                                                                                    \
    template<NUM X>                                                                 \
    friend X operator op (const X &scalar, const CompressedFloat cf) {              \
        return scalar op cf.__get();                                                \
    }                                                                               \

    ARITHMETIC_OP(+)
    ARITHMETIC_OP(-)
    ARITHMETIC_OP(*)
    ARITHMETIC_OP(/)

#undef ARITHMETIC_OP

#define SELF_ARITHMETIC_OP(op)                                                          \
    CompressedFloat &operator STRCAT(op, =) (const CompressedFloat &scalar) {           \
        this->__set(this->__get() op scalar.__get());                                   \
        return *this;                                                                   \
    }                                                                                   \
                                                                                        \
    CompressedFloat operator op (const CompressedFloat &scalar) const {                 \
        return CompressedFloat(*this) STRCAT(op, =) scalar;                             \
    }                                                                                   \

    SELF_ARITHMETIC_OP(+)
    SELF_ARITHMETIC_OP(-)
    SELF_ARITHMETIC_OP(*)
    SELF_ARITHMETIC_OP(/)

#undef SELF_ARITHMETIC_OP

    bool operator !() const {
        return !this->data;
    }

#define COMPARISON_OP(op)                                                           \
    template<NUM X>                                                            \
    bool operator op (const X &scalar) const {                                      \
        return this->__get() op scalar;                                             \
    }                                                                               \
                                                                                    \
    template<NUM X>                                                                 \
    friend bool operator op (const X &scalar, const CompressedFloat &cf) {          \
        return scalar op cf.__get();                                                \
    }                                                                               \

    COMPARISON_OP(<)
    COMPARISON_OP(<=)
    COMPARISON_OP(==)
    COMPARISON_OP(!=)
    COMPARISON_OP(>=)
    COMPARISON_OP(>)

#undef COMPARISON_OP

#define SELF_COMPARISON_OP(op)                              \
    bool operator op (const CompressedFloat &cf) const {    \
        return this->__get() op cf.__get();                 \
    }                                                       \

    SELF_COMPARISON_OP(<)
    SELF_COMPARISON_OP(<=)
    SELF_COMPARISON_OP(==)
    SELF_COMPARISON_OP(!=)
    SELF_COMPARISON_OP(>=)
    SELF_COMPARISON_OP(>)

#undef SELF_COMPARISON_OP

    operator T() const {
        return (T) this->__get();
    }

};

namespace std {
    template<typename _Tp1, typename _Tp2>
    _GLIBCXX_NODISCARD _GLIBCXX14_CONSTEXPR
    inline const _Tp2
    min(const _Tp1 __a, const _Tp2 __b) {
        if (__b < __a)
            return __b;
        return __a;
    }

    template<typename _Tp1, typename _Tp2>
    _GLIBCXX_NODISCARD _GLIBCXX14_CONSTEXPR
    inline const _Tp2
    max(const _Tp1 __a, const _Tp2 __b) {
        if (__a < __b)
            return __b;
        return __a;
    }
}

#undef STRCAT_INNER
#undef STRCAT
