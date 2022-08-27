# Benchmarks around spherical accretion scenario 

This benchmark resembles (though strongly simplified) the setup that was used in the 
MNRAS paper in collaboration with Han Zhang and Baojiu Li. The nice thing about this
benchmark is that it covers a wide range of features even though the actual setup is
comparatively simple.


## Benchmark scenarios

Each benchmark is built in two versions: One optimises the compute kernels for cells
aggressively, where the cell does not have any side effect. The other one skips this
optimisation.

Besides that and the different solver flavours, we have different scenarios:


        ------------------------------------------------------------------
        | scenario | description
        ------------------------------------------------------------------
        | plot     | Very simple regular grid setup, which just helps you 
        |          | to visualise the setup and problem challenge. Coarse 
        |          | mesh and early termination criterion, i.e. not really
        |          | anything interesting here to see.
        ------------------------------------------------------------------
        | reg-0    | Simple regular grid over the unit interval. This is 
        |          | by far the smallest setup. Well-suited for a single 
        |          | node run, it is not really a setup that represents a 
        |          | computation which could yield scientific insight. Too 
        |          | small is the computational domain and the resolution 
        |          | around the overdensity is insufficient to track the 
        |          | evolution if the masses and the arising shocks. 
        ------------------------------------------------------------------

 

## Build executables

For the benchmarks, it is reasonable to build all solvers in release mode without any
plotting functionality. For the standard set of strong scaling tests, I recommend the 
following builds:

     export PYTHONPATH=../../../../python 

     for type in reg-0
     do
       python3 create-benchmark-executable.py --type $type -m release -s fv-4
       python3 create-benchmark-executable.py --type $type -m release -s fv-6
       python3 create-benchmark-executable.py --type $type -m release -s fv-8
       python3 create-benchmark-executable.py --type $type -m release -s dg2-rk2
       python3 create-benchmark-executable.py --type $type -m release -s dg3-rk3
       python3 create-benchmark-executable.py --type $type -m release -s dg4-rk4
     
       python3 create-benchmark-executable.py --type $type -m release -s fv-4 --optimise-stateless
       python3 create-benchmark-executable.py --type $type -m release -s fv-6 --optimise-stateless
       python3 create-benchmark-executable.py --type $type -m release -s fv-8 --optimise-stateless
       python3 create-benchmark-executable.py --type $type -m release -s dg2-rk2 --optimise-stateless
       python3 create-benchmark-executable.py --type $type -m release -s dg3-rk3 --optimise-stateless
       python3 create-benchmark-executable.py --type $type -m release -s dg4-rk4 --optimise-stateless
     done     
     
     
### Underlying core configurations

I usually configure my builds with MPI and a tasking back-end of choice (OpenMP,
TBB, SYCL or C++) and use GPUs for all benchmarks if they are available. As always,
the Python script will pick up the autotool settings from the general Peano build.     

Consult the documentation's appendix (the pdf in the documentation subdirectory) 
for some characteristic settings and module choices that we use quite regularly.
My Hamilton 8 (a Durham machine) setup for example reads
 
      module purge
      module load oneapi
      export FLAVOUR_NOCONFLICT=1
      module load gcc
      module load intelmpi
      export I_MPI_CXX=icpx
     ./configure CC=icpx CXX=icpx CXXFLAGS="-Ofast --std=c++20 -mtune=native -march=native -fma -fomit-frame-pointer -fiopenmp -Wno-unknown-attributes" LDFLAGS="-fiopenmp" --with-multithreading=omp --enable-exahype --enable-loadbalancing --enable-blockstructured --with-mpi=mpiicpc

### Clean-up

If you want to clean up your directory after the builds, run the following 
steps:

       rm Abstract*
       rm Readme*
       rm *.o
       rm Constants.h Makefile benchmark-main.*
       
If you also want to clean up the directories with all the glue code, you can 
invoke      

        rm -rf celldata facedata observers repositories tasks
        


## Run experiments

I recommend to map the runs onto SLURM array scripts. An example script is contained
within the present directory. It is tailored towards the Hamilton supercomputer at 
Durham, but certainly can serve as a baseline script or inspiration for your own 
scripts.


### Copy data back

I solely rely on Peano's pre-manufactured postprocessing scripts for this benchmark 
Consult Peano's documentation (the pdf in the directory with the name documentation)
and its sections on ExaHyPE's output and Peano's IO). Peano's postprocessing scripts
work best with archives over file sets, which also makes it easier to grab data from
the supercomputer if you don't want to postprocess on the machine:


     for type in reg-0
     do
       for solver in FV-4 FV-6 FV-8 RK2DG2 RK3DG3 RK4DG4
       do
         tar -czvf single-node-benchmark-$type-no-opt-$solver.tar.gz single-node-benchmark-$type-no-opt-$solver-*.data
         tar -czvf single-node-benchmark-$type-opt-$solver.tar.gz    single-node-benchmark-$type-opt-$solver-*.data
       done
     done
     
     
All data that does not end with .data (and does not end up in archives) can safely
be removed from the benchmark point of view. We do not evaluate it further here.


      rm *.out
      # assume all is already deployed to tar.gz files
      rm *.data     
     
     
## Postprocess results

The postprocessing here is pretty rudimentary. I use solely the generic default scripts
shipped with Peano. See the documentation for further information as well as the scripts'
help output. While the data might not be presented particularly appealing, it gives a 
good first impression of the code performance:
     

      export PYTHONPATH=../../../../python
      python3 ../../../../python/exahype2/postprocessing/plot-scaling.py


### Single node scaling

We first study the impact of the optimisation on the various solvers:
      
      for type in 0
      do
       for solver in FV-4 FV-6 FV-8 RK2DG2 RK3DG3 RK4DG4
       do
        python3 ../../../../python/exahype2/postprocessing/plot-scaling.py --log-x --log-y --max-cores-per-rank -1 --label not-optimised,optimised --output single-node-assess-optimisation-$type-$solver single-node-benchmark-$type-no-opt-$solver.tar.gz  single-node-benchmark-$type-opt-$solver.tar.gz 
       done
      done
      
If you run these scripts, you obtain a couple plots in pdf and png format which show the 
shared memory scaling behaviour on one node. If you have enabled GPUs, the runs should 
offload to the accelerators, too.


<img src="example-output/single-node-benchmarks-0-FV-4.png" />


We next compare different orders and patch sizes against each other by means of 
scalability. Note that a proper benchmarking from a scientific point of view 
would have to include the obtained accuracy in one way or another, but we focus
exclusively on performance aspects here:
      
      
      for type in 0
      do
       for optimisation in opt no-opt
       do
        python3 ../../../../python/exahype2/postprocessing/plot-scaling.py --log-x --log-y --max-cores-per-rank -1 --label FV-4,FV-6,FV-8 --output single-node-FV-solver-types-$type-$optimisation single-node-benchmark-$type-$optimisation-FV-4.tar.gz single-node-benchmark-$type-$optimisation-FV-6.tar.gz single-node-benchmark-$type-$optimisation-FV-8.tar.gz 
        python3 ../../../../python/exahype2/postprocessing/plot-scaling.py --log-x --log-y --max-cores-per-rank -1 --label RK2DG2,RK3DG3,RK4DG4 --output single-node-RKDG-solver-types-$type-$optimisation single-node-benchmark-$type-$optimisation-RK2DG2.tar.gz single-node-benchmark-$type-$optimisation-RK3DG3.tar.gz single-node-benchmark-$type-$optimisation-RK4DG4.tar.gz        
       done
      done
      

<img src="example-output/single-node-solver-types-0-opt.png" />


### Distribution of threads vs ranks

It is not clear how many ranks one should use per node and, consequently,
how many cores can be used effectively by a rank. This choice, likely, 
depends on the type of NUMA architectures used.

It is hence reasonably to play around with different rank distributions 
before we upscaling further. On AMD's EPYC with 128 cores per node, I 
typically study the following setups:

      number of ranks per node  |  number of cores per rank
      -----------------------------------------------------
      1                         |  128
      2                         |   64
      4                         |   32
      8                         |   16
      16                        |    8
      32                        |    4
      
Note that SLURM refers to tasks, and not to ranks.
