#!/bin/bash

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NORMAL=$(tput sgr0)

# Target value of balsara  and tolerance
TARGET=1.0
TOLERANCE=0.001

printf "Starting density-h solver test. Compiling experiment...\n"
touch test.log
python3 test_density_h_iterative_solver.py > test.log

printf "Running test on a single thread...\n"
export OMP_NUM_THREADS=1
./peano4 >> test.log

printf "Analyzing results...\n"

awk '/rho_sum=/{print $NF}' test.log > temp.out
OUTPUT=$(tail -1 temp.out)

printf "Computed density value: $OUTPUT \n"
printf "TARGET: $TARGET \n"
printf "TOLERANCE: $TOLERANCE \n"

if [ "$( echo "$TARGET - sqrt($OUTPUT*$OUTPUT) < $TOLERANCE" | bc )" = 1 ];
then
  printf "${GREEN}Test passed successfully!${NORMAL}\n"
else
  printf "${RED}Test not successful!${NORMAL}\n"
fi

printf "Cleaning up output files...\n"
rm particles* grid* temp.out 

printf "A log file has been generated.\n"
printf "=============================\n"
exit 0
