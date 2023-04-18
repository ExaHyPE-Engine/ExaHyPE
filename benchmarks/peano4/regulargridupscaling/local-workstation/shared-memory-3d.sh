#!/bin/bash

export d=3
for flops in 1 100 
do
 for h in 0.1 0.05 0.02 
 do
  for cores in 1 2 3 4 6 8 10 12 14 16 20 24 48
  do
   executable="./PeanoRegularGridUpscaling"$d"d"
   output="shared-memory-"$cores"-cores-"$d"d-"$h"-"$flops"-flops.out"
   echo $executable  $output
   timeout 120m $executable $h $flops $cores > $output
  done
 done
done


for flops in 1 100
do
 for h in 0.01 
 do
  for cores in 2 4 6 8 10 12 14 16 20 24 48
  do
   executable="./PeanoRegularGridUpscaling"$d"d"
   output="shared-memory-"$cores"-cores-"$d"d-"$h"-"$flops"-flops.out"
   echo $executable  $output
   timeout 120m $executable $h $flops $cores > $output
  done
 done
done


