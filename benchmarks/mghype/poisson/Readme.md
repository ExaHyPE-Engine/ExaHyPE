/**

 @page benchmarks_mghype_poisson Poisson benchmarks


 For the benchmarks here, we have to set the Python correctly first:
 
<<<<<<< HEAD
       export PYTHONPATH=../../../python:../../../src
=======
       export PYTHONPATH=~/git/Peano/python:~/git/Peano/src
>>>>>>> refs/remotes/origin/multigrid

 After that, all benchmarks can be directly created through the Python scripts.
<<<<<<< HEAD
 You will need the following extensions:
 
 - --enable-mghype 
 - --enable-finiteelements 
 - --with-multithreading=tbb_extension as the OpenMP version of tasking with 
   dependencies still has some bugs.
 - --enable-loadbalancing

 Ensure that you add `LIBS="-ltbb"` in your configure call also.
 
 Three typical runs then are 
 
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 
 MESH_SIZE=0.002  # on Hamilton
 ORDER=5

 python3 benchmarks.py -p $ORDER -m $MESH_SIZE -rea dg-sweeps            -var rediscretise -o mghype-sweeps-rediscretise-p$ORDER
 python3 benchmarks.py -p $ORDER -m $MESH_SIZE -rea dg-shifted-and-fused -var rediscretise -o mghype-shifted-rediscretise-p$ORDER
 python3 benchmarks.py -p $ORDER -m $MESH_SIZE -rea dg-tasks -t 4        -var precompute   -o mghype-tasks-precompute-p$ORDER
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 The first one is the vanilla version of the code, the second one our highly
 optimised one. Both employ solely domain decomposition. The third variant
 adds tasks on top.
 
 
=======

       python3 benchmarks.py -p 6 -m 0.005 -rea dg-sweeps -var rediscretise -o profile 
       
       
>>>>>>> refs/remotes/origin/multigrid
 */
 
 