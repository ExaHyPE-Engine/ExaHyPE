#pragma once

#include <chrono>
#include <map>
#include <numeric>
#include <utility>
#include <cmath>

#include "type.h"

typedef std::chrono::high_resolution_clock::time_point TimeVar;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;

#define duration(a, unit) std::chrono::duration_cast<unit>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()
#define since(a, unit) duration(timeNow() - a, unit)

struct Tracer {
    std::map<const char*, std::vector<double>> samples;

    std::mutex mutex;

    void add(const char* name, double time) {
        this->mutex.lock();
        this->samples[name].push_back(time);
        this->mutex.unlock();
    }

    ~Tracer() {
        for (auto &kv : this->samples) {
            auto *name = kv.first;
            auto &values = kv.second;

            auto minmax = std::minmax_element(values.begin(), values.end());
            auto min = *minmax.first;
            auto max = *minmax.second;

            auto size = values.size();

            if (size == 1) continue;

            auto mean = std::accumulate(values.begin(), values.end(), 0.0) / size;
            auto variance_func = [mean, size](auto accumulator, const auto& val) {
                return accumulator + ((val - mean)*(val - mean) / (size - 1));
            };

            auto variance = std::accumulate(values.begin(), values.end(), 0.0, variance_func);
            auto stdev = sqrt(variance);

            printf("%s (samples: %zu): mean %.2fμs, min %.2fμs, max %.2fμs, st dev %.2fμs\n", name, size, mean, min, max, stdev);
        }
    }
};

#ifndef DISABLE_TRACING
#define TRACE(name, function_call) TimeVar STRCAT(t, __LINE__) = timeNow(); function_call TRACER.add(name, since(STRCAT(t, __LINE__), microseconds)); printf("%s : %ldms\n", name, since(STRCAT(t, __LINE__), milliseconds));
#else
#define TRACE(name, function_call) TimeVar STRCAT(t, __LINE__) = timeNow(); function_call TRACER.add(name, since(STRCAT(t, __LINE__), microseconds));
#endif
