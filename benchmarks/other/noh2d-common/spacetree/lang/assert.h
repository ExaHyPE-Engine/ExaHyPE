#pragma once

#include <cstdlib>
#include <csignal>
#include <cstdio>

#define DEBUGGER {std::raise(SIGINT);}

#define KILL(reason) { fprintf(stderr, "Error: %s\n  %s:%d\n", reason, __FILE__, __LINE__); DEBUGGER std::exit(1); }

#ifndef DNDEBUG
#define ASSERT(cond, reason) if (!(cond)) [[unlikely]] KILL(reason);
#undef assert
#define assert(cond) ASSERT(cond, "Assert failed")
#else
#define ASSERT(cond, reason)
#define assert(cond)
#endif

#define WARN(reason) { fprintf(stderr, "WARN: %s\n  %s:%d\n", reason, __FILE__, __LINE__); }
