#!/bin/bash

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NORMAL=$(tput sgr0)

# Target value of balsara  and tolerance
TARGET=0.8
TOLERANCE=0.001

printf "Starting Balsara switch test. Compiling experiment...\n"
touch test.log
python3 test_balsara_switch_1D.py > test.log

printf "Running test on a single thread...\n"
export OMP_NUM_THREADS=1
./peano4 >> test.log

printf "Analyzing results...\n"

BALSARA=$(awk '/balsara =/{print $NF}' test.log)
printf "Computed balsara switch value: $BALSARA \n"
printf "TARGET: $TARGET \n"
printf "TOLERANCE: $TOLERANCE \n"

if [ "$( echo "$TARGET - sqrt($BALSARA*$BALSARA) < $TOLERANCE" | bc )" = 1 ];
then
  printf "${GREEN}Test passed successfully!${NORMAL}\n"
else
  printf "${RED}Test not successful!${NORMAL}\n"
fi

printf "Cleaning up output files...\n"
rm particles* grid*

printf "A log file has been generated.\n"
printf "=============================\n"
exit 0
