# Regular Grid upscaling studies # 

This example simply constructs a regular grid, and tries to balance it as equal
as possible among all available nodes. After that, each rank (node) splits up 
its domain further into subdomains such that there is one subdomain (tree) per 
core. The code then runs a fixed number of grid sweeps. This allows for core
benchmarking. A command line arguments instructs Peano what grid size has to be
matched at least. The benchmark is available in 2d and 3d.

## Step 1: Prepare executables ##

This step creates the benchmark baseline. Configure you code without any optional
features, i.e. run 

./configure
make clean
make -j
mv src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d-serial
mv src/examples/regulargridupscaling/PeanoRegularGridUpscaling3d src/examples/regulargridupscaling/PeanoRegularGridUpscaling3d-serial

./configure --with-multithreading=cpp --with-mpi=mpicxx
./configure --with-multithreading=tbb --with-mpi=mpicxx
make clean
make -j
mv src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d-parallel
mv src/examples/regulargridupscaling/PeanoRegularGridUpscaling3d src/examples/regulargridupscaling/PeanoRegularGridUpscaling3d-parallel

Alternatively, you can use the debug versions:

mv src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d_debug src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d-parallel
mv src/examples/regulargridupscaling/PeanoRegularGridUpscaling3d_debug src/examples/regulargridupscaling/PeanoRegularGridUpscaling3d-parallel



## Step 2: Single core runs ##

This step creates the benchmark baseline. Switch into the directory holding the executables

cd src/examples/regulargridupscaling

Here, we run the following snippet:


for h in 0.1 0.5 0.01 0.005 0.001
do
  ./PeanoRegularGridUpscaling2d-serial $h > serial2d-$h.out
done


It might be convenient to paste those in to a SLURM file. The same is done for 3d.
After that, we continue with shared and distributed memory runs.

  
## Summary: Submit all the slurm files (on SuperMUC-NG) ##


%sbatch SuperMUC-NG-scripts/serial-2d.slurm-script
%sbatch SuperMUC-NG-scripts/serial-3d.slurm-script

sbatch SuperMUC-NG-scripts/shared-memory-1-core-2d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-2-cores-2d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-3-cores-2d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-6-cores-2d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-12-cores-2d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-24-cores-2d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-48-cores-2d.slurm-script

sbatch SuperMUC-NG-scripts/shared-memory-1-core-3d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-2-cores-3d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-3-cores-3d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-6-cores-3d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-12-cores-3d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-24-cores-3d.slurm-script
sbatch SuperMUC-NG-scripts/shared-memory-48-cores-3d.slurm-script

sbatch SuperMUC-NG-scripts/distributed-memory-1-node-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-2-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-3-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-6-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-8-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-10-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-12-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-14-nodes-2d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-16-nodes-2d.slurm-script

sbatch SuperMUC-NG-scripts/distributed-memory-1-node-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-2-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-3-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-6-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-8-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-10-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-12-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-14-nodes-3d.slurm-script
sbatch SuperMUC-NG-scripts/distributed-memory-16-nodes-3d.slurm-script


## Summary: Submit all the slurm files (on Hamilton) ##

%sbatch Hamilton-scripts/serial-2d.slurm-script
%sbatch Hamilton-scripts/serial-3d.slurm-script

sbatch Hamilton-scripts/shared-memory-1-core-2d.slurm-script
sbatch Hamilton-scripts/shared-memory-2-cores-2d.slurm-script
sbatch Hamilton-scripts/shared-memory-3-cores-2d.slurm-script
sbatch Hamilton-scripts/shared-memory-6-cores-2d.slurm-script
sbatch Hamilton-scripts/shared-memory-12-cores-2d.slurm-script
sbatch Hamilton-scripts/shared-memory-24-cores-2d.slurm-script
sbatch Hamilton-scripts/shared-memory-48-cores-2d.slurm-script

sbatch Hamilton-scripts/shared-memory-1-core-3d.slurm-script
sbatch Hamilton-scripts/shared-memory-2-cores-3d.slurm-script
sbatch Hamilton-scripts/shared-memory-3-cores-3d.slurm-script
sbatch Hamilton-scripts/shared-memory-6-cores-3d.slurm-script
sbatch Hamilton-scripts/shared-memory-12-cores-3d.slurm-script
sbatch Hamilton-scripts/shared-memory-24-cores-3d.slurm-script
sbatch Hamilton-scripts/shared-memory-48-cores-3d.slurm-script

sbatch Hamilton-scripts/distributed-memory-1-node-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-2-nodes-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-4-nodes-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-6-nodes-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-12-nodes-2d.slurm-script

sbatch Hamilton-scripts/distributed-memory-24-nodes-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-36-nodes-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-48-nodes-2d.slurm-script
sbatch Hamilton-scripts/distributed-memory-60-nodes-2d.slurm-script

sbatch Hamilton-scripts/distributed-memory-1-node-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-2-nodes-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-4-nodes-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-6-nodes-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-12-nodes-3d.slurm-script

sbatch Hamilton-scripts/distributed-memory-24-nodes-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-36-nodes-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-48-nodes-3d.slurm-script
sbatch Hamilton-scripts/distributed-memory-60-nodes-3d.slurm-script




/opt/mpi/mpirun -np 1 /opt/intel/inspector/bin64/inspxe-cl -collect ti3 -r r000ti3 /home/tobias/git/Peano/src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d_debug 0.1 1 2

/opt/mpi/mpirun -np 1 /opt/intel/inspector/bin64/inspxe-cl -collect mi3 -knob analyze-stack=true -r r000mi3 /home/tobias/git/Peano/src/examples/regulargridupscaling/PeanoRegularGridUpscaling2d_debug 0.1 1 2

