#include <CL/sycl.hpp>
#include <iostream>
#include <chrono>

using namespace cl::sycl;

int main() {
  int canOffload = 1;
  queue q{gpu_selector{}};

  std::cout << "Device: " << q.get_device().get_info<info::device::name>() << "\n";
  std::cout << "Vendor: " << q.get_device().get_info<info::device::vendor>() << "\n";

  if (q.get_device().is_gpu() && q.get_device().has_extension("cl_khr_fp64")) {
    std::cout << "SYCL offloading is supported." << std::endl;
    canOffload = 0;
  } else {
    std::cout << "SYCL offloading is NOT supported." << std::endl;
    return canOffload;
  }

  const size_t N = 1024 * 1024 * 256;
  std::vector<double> data(N, 1.0);
  buffer<double, 1> buf(data.data(), range<1>(N));

  auto start = std::chrono::high_resolution_clock::now();

  q.submit([&](handler& cgh) {
    auto a = buf.get_access<access::mode::write>(cgh);

    cgh.parallel_for<class bandwidth>(range<1>(N), [=](id<1> i) {
      a[i] *= 2.0;
    });
  });

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  double bandwidth = N * sizeof(double) / diff.count() / 1e9;
  std::cout << "GPU bandwidth: " << bandwidth << " GB/s\n";

  return canOffload;
}
