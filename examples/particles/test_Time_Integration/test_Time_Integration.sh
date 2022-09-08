#!/bin/bash

GREEN=$(tput setaf 2)
NORMAL=$(tput sgr0)


printf "Starting time integration test. Compiling experiment...\n"
rm test.log; touch test.log
python3 test_Time_Integration.py --plot >> test.log

printf "Simulating two-body problem on a single thread...\n"
export OMP_NUM_THREADS=1
./peano4 >> test.log

printf "Analyzing results...\n"
rm energy_conservation_profile.pdf
python3 plots-time-integration-test.py >> test.log

printf "${GREEN}Test passed successfully!${NORMAL}\n"

printf "Cleaning up output files...\n"
rm particles* grid*

printf "A log file has been generated.\n"
printf "A plot has been generated.\n"
printf "=============================\n"
exit 0
