#include <CL/sycl.hpp>
using namespace sycl;

// Input array and output reference of the same type
// Need total number of elements N
// operation is any of these supported std::functional operations
// std::plus<>(), std::multiplies<>(), std::bit_and, bit_or, bit_xor, logical_and, logical_or
// sycl further defines maximum<>() and minimum<>()
template <class T, class F>
void reduce_to_scalar(sycl::queue Q, T * input, T & output, const size_t N, F operation)
{
    T temp(output);
    buffer<T> BUF (&temp, 1);

    Q.submit([&](handler& cgh)
    {
      auto RED = reduction(BUF, cgh, operation);

      cgh.parallel_for(range<1>(N), RED, [=](id<1> i, auto & red)
      {
          red.combine(input[i]);
      });
    }).wait();
    output = BUF.get_host_access()[0];
}




// Reduce an input array of size Nhave to an output array of size Nwant
// This version modifies the input array in case of intermediate steps.
// It is assumed that the input array is "regular" in the sense that
// there are Nhave/Nwant groups of data of the same size that are next to
// each other. The usage of nd_range is therefore crucial.
template <class T, class F>
void reduce_to_array(sycl::queue Q, const size_t Nwant, size_t Nhave, T * input, T * output, F operation)
{
    static const auto WGMAX =  Q.get_device().get_info<info::device::max_work_group_size>();
    while ( (Nhave/Nwant) > WGMAX ) // Keep reducing until the gathersize is <= WGMAX (device specific)
    {
        size_t gathersize(WGMAX);
        while ( not ( (Nhave/gathersize)%Nwant == 0 and (Nhave%gathersize==0) ) )  gathersize--;
        Q.submit([&](handler & cgh)
        {
          cgh.parallel_for(nd_range<1>{{Nhave}, {gathersize}}, [=](nd_item<1> it) 
          {
            input[it.get_group_linear_id()] = reduce_over_group(it.get_group(), input[it.get_global_linear_id()], operation);
          });
        }).wait();
        Nhave /= gathersize;
    }

    // Final reduction
    size_t gathersize = Nhave/Nwant;
    // Note how in the final reduction we can reduce directly into the output buffer
    Q.submit([&](handler & cgh)
    {
      cgh.parallel_for(nd_range<1>{{Nhave}, {gathersize}}, [=](nd_item<1> it) 
      {
        auto wg =  it.get_group();
        output[it.get_group_linear_id()] = reduce_over_group(wg, input[it.get_global_linear_id()], operation);
      });
    }).wait();
}

// Reduce an input array of size Nhave to an output array of size Nwant
// This version does not modify the input array but uses an additional helper array for
// caching of intermediate reduction steps
template <class T, class F>
void reduce_to_array(sycl::queue Q, const size_t Nwant, size_t Nhave, T * input, T * output, T * redbuf, F operation)
{
    static const auto WGMAX =  Q.get_device().get_info<info::device::max_work_group_size>();
    int round(0);
    while ( (Nhave/Nwant) > WGMAX ) // Keep reducing until the gathersize is <= WGMAX (device specific)
    {
        size_t gathersize(WGMAX);
        while ( not ( (Nhave/gathersize)%Nwant == 0 and (Nhave%gathersize==0)) )  gathersize--;
        {
          Q.submit([&](handler & cgh)
          {
            cgh.parallel_for(nd_range<1>{{Nhave}, {gathersize}}, [=](nd_item<1> it) 
            {
              if (round==0) redbuf[it.get_group_linear_id()] = reduce_over_group(it.get_group(),  input[it.get_global_linear_id()], operation);
              else          redbuf[it.get_group_linear_id()] = reduce_over_group(it.get_group(), redbuf[it.get_global_linear_id()], operation);
            });
          }).wait();
        }
        Nhave /= gathersize;
        round++;
    }

    // Final reduction
    size_t gathersize = Nhave/Nwant;
    // Note how in the final reduction we can reduce directly into the output buffer
    Q.submit([&](handler & cgh)
    {
      cgh.parallel_for(nd_range<1>{{Nhave}, {gathersize}}, [=](nd_item<1> it) 
      {
        if (round==0) output[it.get_group_linear_id()] = reduce_over_group(it.get_group(),  input[it.get_global_linear_id()], operation);
        else          output[it.get_group_linear_id()] = reduce_over_group(it.get_group(), redbuf[it.get_global_linear_id()], operation);
      });
    }).wait();
}
