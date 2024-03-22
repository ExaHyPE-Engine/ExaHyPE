#pragma once

#include <cstdint>
#include <concepts>

// https://stackoverflow.com/a/3221914/5769882
#define STRCAT_INNER(a, b) a##b
#define STRCAT(a, b) STRCAT_INNER(a, b)

template<typename X>
concept NUM = std::integral<X> || std::floating_point<X>;

namespace std {
    template<typename CF>
    CF abs(const CF &cf) {
        auto i = cf.__get() > 0;
        if (i) return cf;
        else return -cf;
    }
}

namespace tarch {
namespace la {

    template<typename T, unsigned char BITS = sizeof(T) * 8, unsigned char ALIGNMENT = alignof(T)> requires std::is_floating_point_v<T>
    class alignas(ALIGNMENT) CompressedFloat {
    __attribute__((packed)) std::uint64_t data: BITS;

    T __get() const {
        union {
            std::uint64_t byte_data;
            T float_data;
        } U;

        static_assert(sizeof(T) * 8 >= BITS);

        auto dataLocal = this->data;

        U.byte_data = dataLocal << (sizeof(T) * 8 - BITS);
        auto float_data = U.float_data;
        return float_data;
    }

    void __set(T float_data) {
        union {
            std::uint64_t byte_data;
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

#define UNARY_POSTFIX_OP(op)                            \
    const CompressedFloat operator op(std::int32_t) {   \
        auto oldThis = CompressedFloat(*this);          \
        this->operator op();                            \
        return oldThis;                                 \
    }                                                   \

    UNARY_POSTFIX_OP(++)
    UNARY_POSTFIX_OP(--)

    T operator -() const {
        return -this->__get();
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
    T operator op (const X &scalar) const {                                         \
        return this->__get() op scalar;                                             \
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
    T operator op (const CompressedFloat &scalar) const {                               \
        return this->__get() op scalar;                                                 \
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
    template<NUM X>                                                                 \
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

    template<NUM X>
    operator X() const {
        return (X) this->__get();
    }

    friend CompressedFloat std::abs(const CompressedFloat &cf);

    };

}
}

#undef STRCAT_INNER
#undef STRCAT
