#!/bin/sh

# Script to run experiment

# Parameters
END_TIME=1.0
CUT_OFF_RAD=0.15
HMIN=0.001

# Run python script to generate the experiment
python3 planet-orbit.py -et=$END_TIME --cut-off $CUT_OFF_RAD -hmin $HMIN

# set threads number
export OMP_NUM_THREADS=8

# run experiment
echo 'running experiment...'
./orbit-experiment > output.log

# generate Peano grid for Paraview
pvpython ../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Tidy up root folder and back up results                                                                                                                                                                                                                                    
DIRNAME="exp_r_""$CUT_OFF_RAD"                                                                                                                                                                                                                                               
if [ -d $DIRNAME ]                                                                                                                                                                                                                                                           
then                                                                                                                                                                                                                                                                         
  rm -r $DIRNAME                                                                                                                                                                                                                                                             
fi                                                                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                                             
mkdir $DIRNAME                                                                                                                                                                                                                                                               
mv *.cvs $DIRNAME                                                                                                                                                                                                                                                            
mv Constants.h $DIRNAME                                                                                                                                                                                                                                                      
mkdir $DIRNAME"/snapshots/"                                                                                                                                                                                                                                                  
mv particles* grid* $DIRNAME"/snapshots/"                     

# @TODO: run plot script (e.g. energy conservation plot)
