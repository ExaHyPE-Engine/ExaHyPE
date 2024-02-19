# Quickstart Guide

Peano's build process relies on GNU Autotools or CMake.
The build yields a couple of libraries and executables.
Most codes then use Peano's Python API to build an application on top of the libraries.
You also can create a sole C++ application.
The Python API yields such applications, too, i.e.,
Python is not required to run the simulations but only to construct them.

## Prerequisite

```shell
pip3 install -e python
```

## Autotools

```shell
CC=gcc CXX=g++ python3 configure.py && make -j && make check
```

which is equivalent to

```shell
libtoolize; aclocal; autoconf; autoheader; cp src/config.h.in .; automake --add-missing
CFLAGS="-O3 -march=native" CC="gcc" CXXFLAGS="-std=c++20 -w -O3 -march=native" CXX="g++" LDFLAGS="-march=native" ./configure  && make -j && make check
```

## CMake

```shell
CC=gcc CXX=g++ python3 configure.py --cmake=build && cd build && make -j && make test
```

which is equivalent to


```shell
mkdir build && cd build && CC=gcc CXX=g++ cmake .. && make -j && make test
```

Invoke `make -jX` to use `X` threads to build the code. It is recommended to use slightly less threads than the available number of cores.
Peano is mainly a library collection, i.e.,
you might want to skip the install command and work directly against the libraries built in the source directory.
Use `python3 configure.py --help` to see the available options to configure and build.
Change `CC` and `CXX` to your desired compiler.
For CMake, it is recommended to use `ccmake`.
With this, you can see all available options to interactively enable or disable.
For detailed information on each flag or on advanced build please refer to the documentation.

## Building an Extension

### ExaHyPE 2

#### Autotools

```shell
CC=gcc CXX=g++ python3 configure.py --exahype && make -j && make check
```

which is equivalent to

```shell
libtoolize; aclocal; autoconf; autoheader; cp src/config.h.in .; automake --add-missing
CFLAGS="-O3 -march=native" CC="gcc" CXXFLAGS="-std=c++20 -w -O3 -march=native" CXX="g++" LDFLAGS="-march=native" ./configure --enable-exahype --enable-loadbalancing --enable-blockstructured
make -j && make check
```

#### CMake

```shell
CC=gcc CXX=g++ python3 configure.py --exahype --cmake=build && cd build && make -j && make test
```

which is equivalent to

```shell
mkdir build && cd build
CC=gcc CXX=g++ cmake .. -DENABLE_EXAHYPE=ON -DENABLE_LOADBALANCING=ON -DENABLE_BLOCKSTRUCTURED=ON
make -j && make test
```

#### Build and Run an Application

```shell
cd applications/exahype2/euler
python3 euler.py
```

Run the generated executable.
