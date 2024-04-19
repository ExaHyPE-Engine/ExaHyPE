#pragma once

#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <initializer_list>
#include <iostream>

#include <lang/assert.h>
#include <lang/type.h>


template<typename T, u32 LENGTH>
class NumVec {
    T vec[LENGTH];

public:
    explicit NumVec() = default;

    explicit NumVec(T val) {
        for (u32 i = 0; i < LENGTH; i++) this->vec[i] = val;
    }

    NumVec(std::initializer_list<T> list) {
        assert(list.size() == LENGTH)
        for (u32 i = 0; i < LENGTH; i++) {
            this->vec[i] = list.begin()[i];
        }
    }

    explicit NumVec(const T(&list)[LENGTH]) {
        for (u32 i = 0; i < LENGTH; i++) {
            this->vec[i] = list[i];
        }
    }

    NumVec(const NumVec &other) {
        for (u32 i = 0; i < LENGTH; i++) {
            this->vec[i] = other.vec[i];
        }
    }

    static constexpr u32 size() {
        return LENGTH;
    }

private:
    template<u64 idx>
    void init(T arg0) {
        this->vec[idx] = arg0;
    }

    template<u64 idx, typename... Args>
    void init(T arg0, Args... args) {
        this->init<idx>(arg0);
        this->init<idx+1>(args...);
    }

public:
    template<typename... Args>
    NumVec(T arg0, Args... args) {
        this->init<0>(arg0, args...);
    }

    bool operator==(const NumVec &other) const {
        for (u32 i = 0; i < LENGTH; i++) {
            if (this->vec[i] != other.vec[i]) return false;
        }
        return true;
    }

    bool operator!=(const NumVec &other) const {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
        return !(*this == other);
#pragma clang diagnostic pop
    }

    bool operator>(const NumVec &other) const {
        for (u32 i = 0; i < LENGTH; i++) {
            if (this->vec[i] <= other.vec[i]) return false;
        }
        return true;
    }

    bool operator>=(const NumVec &other) const {
        for (u32 i = 0; i < LENGTH; i++) {
            if (this->vec[i] < other.vec[i]) return false;
        }
        return true;
    }

    bool operator<(const NumVec &other) const {
        for (u32 i = 0; i < LENGTH; i++) {
            if (this->vec[i] >= other.vec[i]) return false;
        }
        return true;
    }

    bool operator<=(const NumVec &other) const {
        for (u32 i = 0; i < LENGTH; i++) {
            if (this->vec[i] > other.vec[i]) return false;
        }
        return true;
    }

    T &operator[](u32 idx) {
        assert(idx < LENGTH)
        return this->vec[idx];
    }

    const T &operator[](u32 idx) const {
        assert(idx < LENGTH)
        return this->vec[idx];
    }

#define SCALAR_OP(op)                                   \
    NumVec operator STRCAT(op, =) (const T &scalar) {   \
        for (u32 i = 0; i < LENGTH; i++) {              \
            this->vec[i] STRCAT(op, =) scalar;          \
        }                                               \
        return NumVec(*this);                           \
    }                                                   \
                                                        \
    NumVec operator op (const T &scalar) const {        \
        return NumVec(*this) STRCAT(op, =) scalar;      \
    }                                                   \


    SCALAR_OP(+)

    SCALAR_OP(-)

    SCALAR_OP(*)

    SCALAR_OP(/)

#undef SCALAR_OP

#define VECTOR_OP(op)                                       \
    NumVec operator STRCAT(op, =) (const NumVec &other) {   \
        for (u32 i = 0; i < LENGTH; i++) {                  \
            this->vec[i] STRCAT(op, =) other.vec[i];        \
        }                                                   \
        return NumVec(*this);                               \
    }                                                       \
                                                            \
    NumVec operator op (const NumVec &other) const {        \
        return NumVec(*this) STRCAT(op, =) other;           \
    }                                                       \


    VECTOR_OP(+)

    VECTOR_OP(-)

    VECTOR_OP(*)

    VECTOR_OP(/)

#undef VECTOR_OP

    bool eq(const NumVec &other, const T eps = std::numeric_limits<T>::epsilon()) const {
        return (*this - other).abs() <= NumVec(eps);
    }

    T sum() const {
        T val = 0;
        for (u32 i = 0; i < LENGTH; i++) val += this->vec[i];
        return val;
    }

    T prod() const {
        T val = 1;
        for (u32 i = 0; i < LENGTH; i++) val *= this->vec[i];
        return val;
    }

    NumVec floor() const {
        auto val = NumVec(*this);
        for (u32 i = 0; i < LENGTH; i++) val[i] = std::floor(val[i]);
        return val;
    }

    NumVec ceil() const {
        auto val = NumVec(*this);
        for (u32 i = 0; i < LENGTH; i++) val[i] = std::ceil(val[i]);
        return val;
    }

    NumVec abs() const {
        auto val = NumVec(*this);
        for (u32 i = 0; i < LENGTH; i++) val[i] = std::abs(val[i]);
        return val;
    }

    NumVec pow(float pow) const {
        auto val = NumVec(*this);
        for (u32 i = 0; i < LENGTH; i++) val[i] = std::pow(val[i], pow);
        return val;
    }

    template<typename Q>
    NumVec<Q, LENGTH> castAs() const {
        NumVec<Q, LENGTH> result;

        for (u32 i = 0; i < LENGTH; i++) {
            result[i] = (Q) this->vec[i];
        }

        return result;
    }

    friend std::ostream &operator<<(std::ostream &stream, const NumVec &value) {
        stream << '[';
        for (u32 i = 0; i < LENGTH - 1; i++) stream << value.vec[i] << ", ";
        stream << value.vec[LENGTH - 1] << ']';

        return stream;
    }

};
