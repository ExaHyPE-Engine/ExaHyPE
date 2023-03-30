#!/bin/sh

# Parameters
END_TIME=0.1
N_PART=560
MANTISSA_SIZE=10

# Save results 
DIRNAME="output_np_"$N_PART"_et_"$END_TIME"_M"$MANTISSA_SIZE
if [ -d $DIRNAME ]
then
  rm -r $DIRNAME
fi

mkdir $DIRNAME
mkdir $DIRNAME"/snapshots/"
mkdir $DIRNAME"/logs_M$MANTISSA_SIZE/"

mv grid* particle* $DIRNAME"/snapshots/"
mv *.log *.err $DIRNAME"/logs_M$MANTISSA_SIZE/"

P4_FILE=p4_output.txt
mv $P4_FILE $DIRNAME

echo "DONE."

