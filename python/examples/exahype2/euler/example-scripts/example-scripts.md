# Some Supercomputer example scripts #



## Single node tests ##

### Prepare Peano 4 core ###

./configure --enable-exahype --with-multithreading=tbb
./configure --enable-exahype --with-multithreading=cpp
./configure --enable-exahype --with-multithreading=omp



### Prepare application ###

See guidebook instructions how to configure the single node environment.
Please remember to add the --with-exahype argument to configure. 

I got plenty of linker errors recently. It was all my fault. If you get linker errors,
ensure you do the following two things:

- Ensure you've re-created the configure environment, as something in the build
  system might have changed (libtoolize; aclocal; autoconf; autoheader; cp src/config.h.in .; automake --add-missing).
- Ensure you've built Peano with the --enable-exahype flag.

It also pays off the delete the main.cpp from time to time.

Next, open the prep script 

vi example-scripts/finitevolumes-with-ExaHyPE2-benchmark.py

and ensure that you've chosen the right solver variant. Build
the codes:

sbatch example-scripts/Hamilton-prepare-executables.slurm-script



### SuperMUC-NG ###

Submit jobs with

sbatch --account=myproject example-scripts/SuperMUC-NG-single-node.slurm-script

So I use for example 

sbatch --account=pr48ma example-scripts/SuperMUC-NG-single-node.slurm-script

### Hamilton ###


sbatch example-scripts/Hamilton-single-node.slurm-script





sbatch example-scripts/Hamilton-1-node-baseline.slurm-script
sbatch example-scripts/Hamilton-1-node.slurm-script
sbatch example-scripts/Hamilton-2-nodes.slurm-script
sbatch example-scripts/Hamilton-3-nodes.slurm-script
sbatch example-scripts/Hamilton-4-nodes.slurm-script
sbatch example-scripts/Hamilton-5-nodes.slurm-script
sbatch example-scripts/Hamilton-6-nodes.slurm-script
sbatch example-scripts/Hamilton-7-nodes.slurm-script
sbatch example-scripts/Hamilton-8-nodes.slurm-script
sbatch example-scripts/Hamilton-10-nodes.slurm-script
sbatch example-scripts/Hamilton-12-nodes.slurm-script
sbatch example-scripts/Hamilton-14-nodes.slurm-script
sbatch example-scripts/Hamilton-16-nodes.slurm-script


## Postprocessing ##

export PYTHONPATH=../../../..

### Shared memory plots ###
python3 ../../../../exahype2/postprocessing/plot-scaling.py results-Hamilton-tbb.tar.gz

### Distributed memory data from Hamilton ###

This is an example script if you try different core counts per rank:

python3 ../../../../exahype2/postprocessing/plot-scaling.py --log --pattern="0.05|0.02|0.01|0.005" --max-cores-per-rank=24  results-Hamilton-distributed-memory.tar.gz

This is an example script if you want to find out which ratio for trees per core is the best one:

python3 ../../../../exahype2/postprocessing/plot-scaling.py --group-measurements=4 --log --pattern="baseline-h-0.05|baseline-h-0.02|baseline-h-0.01|baseline-h-0.01|0.1-h-0.05|0.1-h-0.02|0.1-h-0.01|0.1-h-0.005|0.5-h-0.05|0.5-h-0.02|0.5-h-0.01|0.5-h-0.005|0.7-h-0.05|0.7-h-0.02|0.7-h-0.01|0.7-h-0.005|0.9-h-0.05|0.9-h-0.02|0.9-h-0.01|0.9-h-0.005|0.99-h-0.05|0.99-h-0.02|0.99-h-0.01|0.99-h-0.005" results-Hamilton.tar.gz

### Detailed analysis ###
Unpack the archive and run 

python3 ../../../../../src/toolbox/loadbalancing/plot-load-distribution.py

on the files of interest.

