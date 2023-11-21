#!/bin/bash
#SBATCH --job-name=mem-sort
#SBATCH -p multi
#SBATCH --nodes=1
#SBATCH -c 128
#SBATCH --time=24:00:00
#SBATCH --mail-type=END
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
#SBATCH --array=8,16,32,64

source /etc/profile.d/modules.sh

module purge
module load oneapi/2023.2
export FLAVOUR_NOCONFLICT=1
module load gcc/13.2
module load intelmpi
module load likwid
 
export I_MPI_CXX=icpx
 
export PYTHONPATH=../../../../python

echo Start builds. Ensure you manually delete all executables if your code base has changed
for REALISATION in domain-decomposition.scattered \
                    domain-decomposition.continuous-per-vertex.insert-sorts \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly \
                   domain-decomposition.global-continuous.insert-sorts \
                   domain-decomposition.global-continuous.sort-on-the-fly \
                   domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-all \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-all \
                   domain-decomposition.global-continuous.insert-sorts.vectorise-all \
                   domain-decomposition.global-continuous.sort-on-the-fly.vectorise-all \
                   domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-distance-checks \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-distance-checks \
                   domain-decomposition.global-continuous.insert-sorts.vectorise-distance-checks \
                   domain-decomposition.global-continuous.sort-on-the-fly.vectorise-distance-checks
do
   EXEC_FILE=noh2D-np-2400.$REALISATION
   if (( $SLURM_ARRAY_TASK_ID <= 8 )); then 
     python3 noh.py -np 2400 -plot 0 -et 5e-4 -ppc 1200 -rea $REALISATION --output $EXEC_FILE
   fi
done


for CORES in $SLURM_ARRAY_TASK_ID
do
  echo Start experiments for $CORES cores
  export OMP_NUM_THREADS=$CORES
  for REALISATION in domain-decomposition.scattered \
                     domain-decomposition.continuous-per-vertex.insert-sorts \
                     domain-decomposition.continuous-per-vertex.sort-on-the-fly \
                     domain-decomposition.global-continuous.insert-sorts \
                     domain-decomposition.global-continuous.sort-on-the-fly \
                     domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-all \
                     domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-all \
                     domain-decomposition.global-continuous.insert-sorts.vectorise-all \
                     domain-decomposition.global-continuous.sort-on-the-fly.vectorise-all \
                     domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-distance-checks \
                     domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-distance-checks \
                     domain-decomposition.global-continuous.insert-sorts.vectorise-distance-checks \
                     domain-decomposition.global-continuous.sort-on-the-fly.vectorise-distance-checks
  do
   EXEC_FILE=noh2D-np-2400.$REALISATION
   echo Run ./$EXEC_FILE with $CORES
   likwid-pin -c 0-$CORES ./$EXEC_FILE > noh2D-np-2400-$CORES.$REALISATION.out
   tar -czvf Hamilton8-2400-$OMP_NUM_THREADS-threads.tar.gz noh2D-np-2400-$CORES.*.out
  done
  echo Create tar archive for $OMP_NUM_THREADS threads
  tar -czvf Hamilton8-2400-$OMP_NUM_THREADS-threads.tar.gz noh2D-np-2400-$CORES.*.out
done


