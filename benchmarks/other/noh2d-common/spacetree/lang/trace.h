#pragma once

#include <chrono>
#include <utility>

#include <lang/type.h>

typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a) std::chrono::duration_cast<std::chrono::milliseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()
#define since(a) duration(timeNow() - a)

#ifndef DISABLE_TRACING
#define TRACE(name, function_call) TimeVar STRCAT(t, __LINE__) = timeNow(); function_call printf("%s : %ldms\n", name, since(STRCAT(t, __LINE__)));
#else
#define TRACE(name, function_call) function_call
#endif
