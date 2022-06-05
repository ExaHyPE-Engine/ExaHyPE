#include <gtest/gtest.h>
#include <stdexcept>
#include <numeric>
#include "reductionlibrary.h"

#include <CL/sycl.hpp>
using namespace sycl;


static queue mQ(default_selector{});


TEST(ReductionLibraryTest, BasicAssertions)
{
    const size_t N(100);
    auto input = malloc_shared<int>(N, mQ);
    for (int i=0;i<N;i++) input[i] = i+1;

    // initial value
    int sum(0);
    int max(-1);
    //reduce<double>(input, output, N);
    reduce_to_scalar<int>(mQ, input, sum, N, plus<>());
    reduce_to_scalar<int>(mQ, input, max, N, maximum<>());

    EXPECT_EQ(sum, 5050);
    EXPECT_EQ(max, 100);

    free(input, mQ);
}


TEST(ReductionLibraryTest, Check3D)
{
  std::vector<size_t> nvalues = {2,3,4,5,8,9,12,30,31};
  const size_t N(201);

  for (auto n : nvalues)
  {
    size_t nitems(3*n*n*n*N);
    auto input = malloc_shared<double>(nitems,mQ);
    for (int i =0;i<nitems;i++) input[i] = i;
    
    std::vector<double> test(nitems);
    std::iota(test.begin(), test.end(), 0);

    auto output = malloc_shared<double>(N,mQ);
    for (int i =0;i<N;i++) output[i] = -1;

    auto redbuf = malloc_device<double>(nitems, mQ); 
    
    reduce_to_array<double>(mQ, N, nitems, input, output, redbuf, maximum<>());
    for (int i=0;i<N;i++)
    {
      EXPECT_EQ(output[i], *std::max_element(test.begin() + 3*n*n*n*i, test.begin() + 3*n*n*n*(i+1)));
    }

    reduce_to_array<double>(mQ, N, nitems, input, output, maximum<>());
    for (int i=0;i<N;i++)
    {
      EXPECT_EQ(output[i], *std::max_element(test.begin() + 3*n*n*n*i, test.begin() + 3*n*n*n*(i+1)));
    }
    
    free(redbuf,  mQ);
    free(input,   mQ);
    free(output,  mQ);
  }
}

TEST(ReductionLibraryTest, Check2D)
{
  std::vector<size_t> nvalues = {2,3,4,5,8,9,12,30,31,40,45,100};
  const size_t N(201);

  for (auto n : nvalues)
  {
    size_t nitems(2*n*n*N);
    auto input = malloc_shared<double>(nitems,mQ);
    for (int i =0;i<nitems;i++) input[i] = i;
    
    std::vector<double> test(nitems);
    std::iota(test.begin(), test.end(), 0);

    auto output = malloc_shared<double>(N,mQ);
    for (int i =0;i<N;i++) output[i] = -1;

    auto redbuf = malloc_device<double>(nitems, mQ); 
    
    reduce_to_array<double>(mQ, N, nitems, input, output, redbuf, maximum<>());
    for (int i=0;i<N;i++)
    {
      EXPECT_EQ(output[i], *std::max_element(test.begin() + 2*n*n*i, test.begin() + 2*n*n*(i+1)));
    }

    reduce_to_array<double>(mQ, N, nitems, input, output, maximum<>());
    for (int i=0;i<N;i++)
    {
      EXPECT_EQ(output[i], *std::max_element(test.begin() + 2*n*n*i, test.begin() + 2*n*n*(i+1)));
    }
    
    free(redbuf,  mQ);
    free(input,   mQ);
    free(output,  mQ);
  }
}
