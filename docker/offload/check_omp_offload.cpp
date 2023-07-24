#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <omp.h>

int main() {
  int canOffload = 1;
  #pragma omp target map(tofrom : canOffload)
  {
    if (!omp_is_initial_device()) {
      canOffload = 0;
    }
  }
  if (canOffload == 0) {
    std::cout << "OpenMP offloading is supported." << std::endl;
  } else {
    std::cout << "OpenMP offloading is NOT supported." << std::endl;
    return canOffload;
  }

  int numProcs = omp_get_num_procs();
  int maxThreads = omp_get_max_threads();
  std::cout << "omp_get_num_procs: " << numProcs << ", omp_get_max_threads: " << maxThreads << std::endl;

  std::vector<double> timings;
  constexpr int N = 33554432*2;
  double* a = static_cast<double*>(aligned_alloc(16, sizeof(double) * N));
  double* b = static_cast<double*>(aligned_alloc(16, sizeof(double) * N));
  #pragma omp target enter data map(alloc : a [0:N], b [0:N])

  std::chrono::high_resolution_clock::time_point t1, t2;
  for (unsigned int k = 0; k < 100; k++) {
    t1 = std::chrono::high_resolution_clock::now();
    //#pragma omp target loop
    #pragma omp target teams distribute parallel for simd
    for (int i = 0; i < N; i++) {
      b[i] = a[i];
    }
    t2 = std::chrono::high_resolution_clock::now();
    timings.push_back(std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count());
  }

  #pragma omp target exit data map(release : a [0:N], b [0:N])
  free(a);
  free(b);

  const double minTime  = *std::minmax_element(timings.begin() + 1, timings.end()).first;
  const double bandwidth = 1.0E-9 * 2 * sizeof(double) * N / minTime;
  std::cout << minTime << " [s] / " << bandwidth << " [GB/s]" << std::endl;
  return canOffload;
}
