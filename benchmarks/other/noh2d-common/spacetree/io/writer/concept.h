#pragma once

#include <lang/type.h>

template<typename T>
concept IoWriter = requires(T t) {
    { t.write(Any()) } -> std::same_as<void>;
    { t.write(void_ptr(), u64()) } -> std::same_as<void>;
    { t.seek(u64()) } -> std::same_as<void>;
    { t.sync() } -> std::same_as<void>;
    { t.getPos() } -> std::same_as<u64>;
};

template<typename T>
concept IoBufferedWriter = IoWriter<T> && requires(T t) {
    { t.buffer(u64()) } -> std::same_as<void*>;
};
