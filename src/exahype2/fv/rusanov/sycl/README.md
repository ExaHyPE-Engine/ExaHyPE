# Google test

Google test by default uses Werror. Due to the target triple warning being
unsolved as for now, we need to remove the flag from the googletest configuration.
Since we pull googletest from github, we need to use *execute_process* in CMakeList.txt.
Somehow this does not trigger correctly, so the poor man's solution is to simply run
cmake twice as sketched below.

```
mkdir build
cd build
cmake ../ -DCMAKE_CXX_COMPILER=clang++
cmake ../ -DCMAKE_CXX_COMPILER=clang++
make
make test
```
