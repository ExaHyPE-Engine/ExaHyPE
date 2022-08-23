# Benchmarks around spherical accretion scenario 

This benchmark resembles (though strongly simplified) the setup that was used in the 
MNRAS paper in collaboration with Han Zhang and Baojiu Li. The nice thing about this
benchmark is that it covers a wide range of features even though the actual setup is
comparatively simple.


## Build executables

For the benchmarks, it is reasonable to build all solvers in release mode without any
plotting functionality. For the standard set of strong scaling tests, I recommend the 
following builds:

     export PYTHONPATH=../../../../python 

     python3 create-benchmark-executable.py -m release -s fv-4
     python3 create-benchmark-executable.py -m release -s fv-6
     python3 create-benchmark-executable.py -m release -s fv-8
     python3 create-benchmark-executable.py -m release -s dg2-rk2
     python3 create-benchmark-executable.py -m release -s dg3-rk3
     python3 create-benchmark-executable.py -m release -s dg4-rk4


## Run experiments

I recommend to map the runs onto SLURM array scripts. An example script is contained
within the present directory. It is tailored towards the Hamilton supercomputer at 
Durham, but certainly can serve as a baseline script or inspiration for your own 
scripts.




