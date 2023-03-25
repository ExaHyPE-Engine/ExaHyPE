#!/bin/sh

# Parameters
END_TIME=0.01
N_PART=400
CLANG_STATUS="on"

# Save results 
DIRNAME="output_np_"$N_PART"_et_"$END_TIME"_"$CLANG_STATUS

P4_FILE="p4_output_"$CLANG_STATUS".tar.gz"
tar -cvzf $P4_FILE p4_*.txt 
rm p4_*.txt

mkdir $DIRNAME
mkdir $DIRNAME"/likwid/"
mkdir $DIRNAME"/logs_$CLANG_STATUS/"
mv *.cvs $DIRNAME"/likwid/"
mv $P4_FILE $DIRNAME
mv *.log *.err $DIRNAME"/logs_$CLANG_STATUS/"

# Run exahype post-processing script to parse log file
#./run_scaling_plot_exahype.sh $CLANG_STATUS $N_PART $DIRNAME

echo "DONE."
