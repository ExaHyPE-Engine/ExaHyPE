#include <iostream>

int main() {
  int a = 2, b = 3, c = 0;

  #pragma acc parallel copy(c) copyin(a, b)

  #pragma acc
  c = a + b;
  if (c == 5) {
    std::cout << "OpenACC offloading is supported." << std::endl;
    return 0;
  } else {
    std::cout << "OpenACC offloading is NOT supported." << std::endl;
    return 1;
  }
}
