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

     python3 create-benchmark-executable.py --type 0 -m release -s fv-4
     python3 create-benchmark-executable.py --type 0 -m release -s fv-6
     python3 create-benchmark-executable.py --type 0 -m release -s fv-8
     python3 create-benchmark-executable.py --type 0 -m release -s dg2-rk2
     python3 create-benchmark-executable.py --type 0 -m release -s dg3-rk3
     python3 create-benchmark-executable.py --type 0 -m release -s dg4-rk4


## Run experiments

I recommend to map the runs onto SLURM array scripts. An example script is contained
within the present directory. It is tailored towards the Hamilton supercomputer at 
Durham, but certainly can serve as a baseline script or inspiration for your own 
scripts.

I solely rely on Peano's pre-manufactured postprocessing scripts for this benchmark 
Consult Peano's documentation (the pdf in the directory with the name documentation)
and its sections on ExaHyPE's output and Peano's IO). Peano's postprocessing scripts
work best with archives over file sets, which also makes it easier to grab data from
the supercomputer if you don't want to postprocess on the machine:


     tar -czvf single-node-benchmark-FV-4.tar.gz single-node-benchmark-FV-4*.data
     tar -czvf single-node-benchmark-FV-6.tar.gz single-node-benchmark-FV-6*.data
     tar -czvf single-node-benchmark-FV-8.tar.gz single-node-benchmark-FV-8*.data
     tar -czvf single-node-benchmark-RK2DG2.tar.gz single-node-benchmark-RK2DG2*.data
     tar -czvf single-node-benchmark-RK3DG3.tar.gz single-node-benchmark-RK3DG3*.data
     tar -czvf single-node-benchmark-RK4DG4.tar.gz single-node-benchmark-RK4DG4*.data
     
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

      
      
      python3 ../../../../python/exahype2/postprocessing/plot-scaling.py --log-x --log-y --max-cores-per-rank -1 --label FV-4,FV-6,FV-8 --output single-node-benchmarks-FV single-node-benchmark-FV-4.tar.gz single-node-benchmark-FV-6.tar.gz single-node-benchmark-FV-8.tar.gz
      python3 ../../../../python/exahype2/postprocessing/plot-scaling.py --log-x --log-y --max-cores-per-rank -1 --label RK2DG2,RK3DG3,RK4DG4 --output single-node-benchmarks-RKDG single-node-benchmark-RK2DG2.tar.gz single-node-benchmark-RK3DG3.tar.gz single-node-benchmark-RK4DG4.tar.gz


