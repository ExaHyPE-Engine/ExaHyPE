#!/bin/bash

export d=2
for flops in 1 100 
do
 for h in 0.01 0.005 0.002 0.001
 do
  for cores in 1 2 3 4 6 8 10 12 14 16 20 24 48
  do
   executable="./PeanoRegularGridUpscaling"$d"d"
   output="shared-memory-"$cores"-cores-"$d"d-"$h"-"$flops"-flops.out"
   echo $executable  $output
   timeout 45m $executable $h $flops $cores > $output
  done
 done
done



