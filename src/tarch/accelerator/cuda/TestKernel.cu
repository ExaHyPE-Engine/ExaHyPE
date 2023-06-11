
#if defined(GPUOffloadingCUDA)

#include <vector>
#include <stdio.h>
#include <iostream>

__global__ void testKernelBody(int* a){
    (*a)++;
}

std::vector<bool> testKernelLaunchCUDACall(){
    int  numberOfDevices;
    cudaGetDeviceCount(&numberOfDevices);

    std::vector<bool> offloadResults(numberOfDevices, false);
    int savedCurrentDevice{0};


    cudaGetDevice(&savedCurrentDevice);

    for (int i = 0; i < numberOfDevices; i++){
        cudaSetDevice(i);
        int *dev_counter;
        cudaMalloc(&dev_counter, sizeof(int));
        cudaMemset(dev_counter, i, sizeof(int));
        testKernelBody<<<32,32>>>(dev_counter);
        cudaDeviceSynchronize();
        cudaError_t err = cudaGetLastError();
        offloadResults[i] = err == cudaSuccess;
        cudaDeviceReset();
    }

    cudaSetDevice(savedCurrentDevice);

    return offloadResults;
}

#endif