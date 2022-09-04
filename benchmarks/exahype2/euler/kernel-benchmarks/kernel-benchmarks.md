# Selection of benchmarks for the Euler compute kernels

This directory hosts a set of benchmarks which assess the sole performance of 
the individual kernels on the CPU and GPUs. While this might be of limited 
relevance for large-scale runs, where scalability, IO overheads and other
factors play a dominant role, it tells us what the core compute routines could,
in theory, deliver.

## Build benchmarks

There are different benchmarks for the different solver types (currently finite
volumes and discontinuous Galerking) and you have to build different executables
for different patch sizes, orders or dimensions. Below is a set of benchmarks 
that I tend to build from time to time.


### Finite Volumes


### Clean up

You can throw away all of the glue code after a successful build. This is 
triggered by
    
    
     rm Abstract* Readme* Makefile Constants.h
     rm -rf celldata facedata observers repositories tasks
     rm *.o
     
     
     
## Runs

All the benchmarks run out of the box without any user interaction once built.
All of them produce outputs in a similar way, so it is easier to interpret the
outcomes. For most runs, you don't have to specify anything. However, you might
want to call

      ./my-benchmark --help
      
It enlists the standard ExaHyPE command line arguments that every code has. Most
of them play no role here, but the thread count and the gpu filters do, i.e. you
can ensure that we use a certain thread count only, and you can pick the GPUs 
that the benchmark should assess. 

I typically run the 



Note the slightly different semantics of the GPU setting for SYCL and OpenMP: 
For SYCL, you explicitly have to specify which GPUs you want to use. You can 
pass all as argument, but you have to pass something. Otherwise, ExaHyPE will
only use the host. OpenMP in contrast uses all the devices that are available
by default. 


## Visualiation of results

If you want to use the pre-manufactured visualisation scripts, you have to pipe
the program output into a file. 



