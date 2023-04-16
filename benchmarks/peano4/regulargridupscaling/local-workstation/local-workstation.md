




## Create shared memory benchmark data ##

./configure --with-multithreading=cpp
make clean; rm src/examples/regulargridupscaling/PeanoRegularGridUpscaling*
make -j
cd src/examples/regulargridupscaling
chmod a+x local-workstation/*.sh
local-workstation/shared-memory-2d.sh
local-workstation/shared-memory-3d.sh



## Create plots ##

python3 create-plots.py create-plots shared-memory


