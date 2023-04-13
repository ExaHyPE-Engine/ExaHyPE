#!/bin/bash

PS3="Select toolchain: "
select toolchain in gcc llvm nvidia amd intel; do
  case $toolchain in
    "gcc")
      break;;
    "llvm")
      break;;
    "nvidia")
      break;;
    "amd")
      break;;
    "intel")
      break;;
  esac
done

if [ "$toolchain" = "amd" ]; then
  read -p "Enter CC compiler [amdclang, hipcc]: " cc
  cc=${cc:-amdclang}
  read -p "Enter CXX compiler [amdclang++, hipcc]: " cxx
  cxx=${cxx:-amdclang++}
  #read -p "Enter Fortran compiler: " fcc
  CXXFLAGS="-std=c++20 -O3 -W -Wall -Wextra"
  LDFLAGS=""
  PS3="Select multithreading mode: "
  select shared in none omp; do
    case $shared in
      "none")
        break;;
      "omp")
        break;;
    esac
  done
  if [ ! "$shared" = "none" ]; then
    CXXFLAGS+=" -fopenmp"
    LDFLAGS+=" -fopenmp"
    PS3="Select GPU offloading mode: "
    select gpu in none omp; do
      case $gpu in
        "none")
          break;;
        "omp")
          break;;
      esac
    done
    if [ ! "$gpu" = "none" ]; then
      PS3="Select GPU compute capability: "
      select cap in gfx906 gfx908 gfx90a; do
        case $cap in
          "gfx906") # MI 50
            break;;
          "gfx908") # MI 100
            break;;
          "gfx90a") # MI 200
            break;;
        esac
      done
      CXXFLAGS+=" --offload-arch=$cap" # -x hip -target amdgcn-amd-amdhsa
      LDFLAGS+=" --offload-arch=$cap -lstdc++fs"
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-particles \
        --enable-fem \
        --enable-swift \
        --with-fenv \
        --with-gpu=$gpu \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    else
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-particles \
        --enable-fem \
        --enable-swift \
        --with-fenv \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    fi
  else
    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"
    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-particles \
      --enable-fem \
      --enable-swift \
      --with-fenv \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi

if [ "$toolchain" = "nvidia" ]; then
  read -p "Enter CC compiler [nvc]: " cc
  cc=${cc:-nvc}
  read -p "Enter CXX compiler [nvc++]: " cxx
  cxx=${cxx:-nvc++}
  #read -p "Enter Fortran compiler: " fcc
  CXXFLAGS="-std=c++20 -O4 -W -Wall -Wextra"
  LDFLAGS=""
  PS3="Select multithreading mode: "
  select shared in none omp cpp; do
    case $shared in
      "none")
        break;;
      "omp")
        break;;
      "cpp")
        break;;
    esac
  done
  if [ ! "$shared" = "none" ]; then
    PS3="Select GPU offloading mode: "
    select gpu in none omp cpp; do
      case $gpu in
        "none")
          break;;
        "omp")
          break;;
        "cpp")
          break;;
      esac
    done
    if [ ! "$gpu" = "none" ]; then
      PS3="Select GPU compute capability: "
      select cap in cc70 cc72 cc75 cc80 cc86 cc90; do
        case $cap in
          "cc70")
            break;;
          "cc72")
            break;;
          "cc75")
            break;;
          "cc80")
            break;;
          "cc86")
            break;;
          "cc90")
            break;;
        esac
      done
      CXXFLAGS+=" -mp=gpu -gpu=$cap"
      LDFLAGS+=" -mp=gpu -gpu=$cap"
      PS3="Do you want to enable CUDA?: "
      select cuda in yes no; do
          case $cuda in
          "yes")
            CXXFLAGS+=" -cuda"
            LDFLAGS+=" -cuda"
            break;;
          "no")
            break;;
        esac
      done
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-particles \
        --enable-fem \
        --enable-swift \
        --with-fenv \
        --with-gpu=$gpu \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    else
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-particles \
        --enable-fem \
        --enable-swift \
        --with-fenv \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    fi
  else
    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"
    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-particles \
      --enable-fem \
      --enable-swift \
      --with-fenv \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi

if [ "$toolchain" = "gcc" ]; then
  read -p "Enter CC compiler [gcc]:" cc
  cc=${cc:-gcc}
  read -p "Enter CXX compiler: [g++]" cxx
  cxx=${cxx:-g++}
  #read -p "Enter Fortran compiler: " fcc
  CXXFLAGS="-std=c++20 -W -Wall -Wextra"
  LDFLAGS=""
  PS3="Select multithreading mode: "
  select shared in none omp; do
    case $shared in
      "none")
        break;;
      "omp")
        break;;
    esac
  done
  if [ ! "$shared" = "none" ]; then
    CXXFLAGS+=" -fopenmp"
    LDFLAGS+=" -fopenmp"
    PS3="Select GPU offloading mode: "
    select gpu in none omp; do
      case $gpu in
        "none")
          break;;
        "omp")
          break;;
      esac
    done
    if [ ! "$gpu" = "none" ]; then
      CXXFLAGS+=" -foffload=nvptx-none"
      LDFLAGS+=" -foffload=nvptx-none"
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-fem \
        --with-fenv \
        --with-gpu=$gpu \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    else
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-particles \
        --enable-fem \
        --enable-swift \
        --with-fenv \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    fi
  else
    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"
    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-particles \
      --enable-fem \
      --enable-swift \
      --with-fenv \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi

if [ "$toolchain" = "intel" ]; then
  read -p "Enter CC compiler [icx]: " cc
  cc=${cc:-icx}
  read -p "Enter CXX compiler [icpx]: " cxx
  cxx=${cxx:-icpx}
  #read -p "Enter Fortran compiler: " fcc
  CXXFLAGS="-std=c++20 -W -Wall -Wextra"
  LDFLAGS=""
  PS3="Select multithreading mode: "
  select shared in none omp tbb sycl; do
    case $shared in
      "none")
        break;;
      "omp")
        break;;
      "tbb")
        break;;
      "sycl")
        break;;
    esac
  done
  if [ ! "$shared" = "none" ]; then
    CXXFLAGS+=" -fiopenmp"
    LDFLAGS+=" -fiopenmp"
    PS3="Select GPU offloading mode: "
    select gpu in none omp sycl; do
      case $gpu in
        "none")
          break;;
        "omp")
          break;;
        "sycl")
          break;;
      esac
    done
    if [ ! "$gpu" = "none" ]; then
      PS3="Select GPU compute capability: "
      select cap in sm_70 sm_72 sm_75 sm_80 sm_86 sm_90 spir64; do
        case $cap in
          "sm_70")
            break;;
          "sm_72")
            break;;
          "sm_75")
            break;;
          "sm_80")
            break;;
          "sm_86")
            break;;
          "sm_90")
            break;;
          "spir64")
            break;;
        esac
      done
      CXXFLAGS+=" -fsycl -Xsycl-target-frontend -fgpu-inline-threshold=100000 -Wno-unknown-cuda-version -Wno-deprecated-declarations -Wno-linker-warnings -Wno-unknown-attributes -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend --cuda-gpu-arch=$cap" \
      LDFLAGS+=" -fsycl -Xsycl-target-frontend -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend --cuda-gpu-arch=$cap -lstdc++fs"
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-fem \
        --with-fenv \
        --with-gpu=$gpu \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    else
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-particles \
        --enable-fem \
        --enable-swift \
        --with-fenv \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    fi
  else
    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"
    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-particles \
      --enable-fem \
      --enable-swift \
      --with-fenv \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi

if [ "$toolchain" = "llvm" ]; then
  read -p "Enter CC compiler [clang]: " cc
  cc=${cc:-clang}
  read -p "Enter CXX compiler [clang++]: " cxx
  cxx=${cxx:-clang++}
  #read -p "Enter Fortran compiler: " fcc
  CXXFLAGS="-std=c++17 -W -Wall -Wextra" #-flto"
  LDFLAGS=""
  PS3="Select multithreading mode: "
  select shared in none omp; do
    case $shared in
      "none")
        break;;
      "omp")
        break;;
    esac
  done
  if [ ! "$shared" = "none" ]; then
    CXXFLAGS+=" -fopenmp"
    PS3="Select GPU offloading mode: "
    select gpu in none omp; do
      case $gpu in
        "none")
          break;;
        "omp")
          break;;
      esac
    done
    if [ ! "$gpu" = "none" ]; then
      PS3="Select GPU compute capability: "
      select cap in sm_70 sm_72 sm_75 sm_80 sm_86 sm_90; do
        case $cap in
          "sm_70")
            break;;
          "sm_72")
            break;;
          "sm_75")
            break;;
          "sm_80")
            break;;
          "sm_86")
            break;;
          "sm_90")
            break;;
        esac
      done
      CXXFLAGS+=" -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$cap -Wno-unknown-cuda-version"
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-fem \
        --with-fenv \
        --with-gpu=$gpu \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    else
      echo "$cc $CXXFLAGS"
      echo "$cxx $CXXFLAGS"
      echo "LDFLAGS: $LDFLAGS"
      libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
      ./configure \
        --enable-exahype \
        --enable-loadbalancing \
        --enable-blockstructured \
        --enable-fem \
        --with-fenv \
        --with-multithreading=$shared \
        CC=$cc CXX=$cxx \
        CXXFLAGS="$CXXFLAGS" \
        LDFLAGS="$LDFLAGS"
    fi
  else
    echo "$cc $CXXFLAGS"
    echo "$cxx $CXXFLAGS"
    echo "LDFLAGS: $LDFLAGS"
    libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing && \
    ./configure \
      --enable-exahype \
      --enable-loadbalancing \
      --enable-blockstructured \
      --enable-fem \
      --with-fenv \
      CC=$cc CXX=$cxx \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS"
  fi
fi
