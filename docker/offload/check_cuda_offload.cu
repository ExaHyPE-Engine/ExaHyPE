#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <cstring>

void checkError() {
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::cerr << "Error: " << cudaGetErrorString(err) << std::endl;
    exit(err);
  }
}

std::string getDeviceName(const int device) {
  cudaDeviceProp props;
  cudaGetDeviceProperties(&props, device);
  checkError();
  return std::string(props.name);
}

std::string getDeviceDriver(const int device) {
  cudaSetDevice(device);
  checkError();
  int driver;
  cudaDriverGetVersion(&driver);
  checkError();
  return std::to_string(driver);
}

__global__ void kernel(const double* a, double* b) {
  const int i = blockDim.x * blockIdx.x + threadIdx.x;
  b[i]        = a[i];
}

int main() {
  checkError();
  cudaSetDevice(0);
  checkError();
  cudaDeviceProp props;
  cudaGetDeviceProperties(&props, 0);

  constexpr int N = 33554432*2;
  if (props.totalGlobalMem < 2 * N * sizeof(double))
    throw std::runtime_error("Device does not have enough memory for all 2 buffers");
  double *d_a, *d_b;
  cudaMalloc(&d_a, N * sizeof(double));
  checkError();
  cudaMalloc(&d_b, N * sizeof(double));
  checkError();

  std::vector<double> timings;
  for (unsigned int k = 0; k < 100; k++) {
    cudaEvent_t t1, t2;
    cudaEventCreate(&t1);
    cudaEventCreate(&t2);
    cudaEventRecord(t1);
    kernel<<<N/1024, 1024>>>(d_a, d_b);
    cudaEventRecord(t2);
    cudaEventSynchronize(t2);
    checkError();
    cudaDeviceSynchronize();
    checkError();
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, t1, t2);
    timings.push_back(milliseconds / 1000.0);
  }

  cudaFree(d_a);
  checkError();
  cudaFree(d_b);
  checkError();

  const double minTime  = *std::minmax_element(timings.begin() + 1, timings.end()).first;
  const double bandwidth = 1.0E-9 * 2 * sizeof(double) * N / minTime;
  std::cout << minTime << " [s] / " << bandwidth << " [GB/s]" << std::endl;
  return 0;
}
