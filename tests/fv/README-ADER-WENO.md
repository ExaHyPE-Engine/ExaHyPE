# ADER-WENO on the new engine

`exahype2.fv.Weno(order=p)` selects a degree-`p` WENO polynomial and the
ADER space-time predictor. Supported degrees are 2–5. A patch needs at least
`2*p` volumes per axis and uses `p+1` halo layers. The same scheme can be
passed to `add_subcell_limiter()`; its degree must match the DG degree.

The new engine stores face layers contiguously and dispatches individual
volume updates on the GPU. Each ADER worker reconstructs and predicts the
states needed by its volume, using private scratch storage. Reconstructions
at a patch boundary use the common stencil of the two patches so that their
fluxes agree. Predictor storage follows the padded DG layout, while patch
and face values use the engine's ordinary unpadded layout.

## Build and test with CUDA

From the repository root, with MPI, OpenMP and CUDA available:

```sh
cmake -S . -B build-ader-weno-cuda -G Ninja \
  -DSUPPORTED_BUILD_TYPES=Asserts -DSUPPORTED_DIMENSIONS='2;3' \
  -DPEANO_WITH_GPU_BACKEND=native -DPEANO_WITH_GPU_ARCH=sm_89
cmake --build build-ader-weno-cuda \
  --target UnitTestsAsserts2D UnitTestsAsserts3D -j 6
ctest --test-dir build-ader-weno-cuda \
  -R '^UnitTestsAsserts[23]D$' --output-on-failure
```

`sm_89` targets an RTX 4090. Select the architecture and CUDA/host compiler
paths appropriate to the machine. For a CPU build, use
`-DPEANO_WITH_GPU_BACKEND=none` in a separate build directory.

When several CUDA toolkits are installed, set both `CMAKE_CUDA_COMPILER`
and `CUDAToolkit_ROOT` to the same installation, for example
`-DCMAKE_CUDA_COMPILER=/usr/local/cuda-13.0/bin/nvcc`
`-DCUDAToolkit_ROOT=/usr/local/cuda-13.0`. Use a fresh build directory when
switching toolkits so that cached CUDA libraries cannot mismatch the headers.

`ADERWENOTest` checks polynomial reconstruction, exponential source
integration, conservative and nonconservative advection convergence, and
periodic mass conservation. It also compares independent device volume
updates against host patch sweeps in double, float and mixed precision,
including auxiliary variables and degree-three reconstruction.

## Generated applications

Set `PYTHONPATH` to the repository's `api` directory and `PEANO_BUILD_DIR`
to the absolute core build directory. Generate each application in its own
working directory to keep its output separate.

Standalone finite volumes:

```sh
python3 /path/to/Peano/tests/fv/rayleigh-taylor-instability.py \
  -m Asserts -j 4 --end-time 0.002 --degrees-of-freedom 4 \
  --grid-levels 2 --solver ader-weno --weno-order 2
OMP_NUM_THREADS=4 ./ExaHyPE
```

ADER-DG with every cell forced through the WENO subcell limiter:

```sh
python3 /path/to/Peano/tests/aderdg-limiting/euler-rayleigh-taylor.py \
  -m Asserts -j 4 --end-time 0.002 --degrees-of-freedom 3 \
  --min-grid-levels 2 --max-grid-levels 2 \
  --limiter-solver weno --limiter-mode all-limited
OMP_NUM_THREADS=4 ./ExaHyPE
```

## Merge validation

Validated on an RTX 4090 with CUDA 13.0 and GCC 14 as the CUDA host compiler:

- `UnitTestsAsserts2D` and `UnitTestsAsserts3D` passed on CPU and CUDA
  (one MPI rank, four OpenMP threads; the CPU build used GCC 15).
- Both generated applications above completed to `t=0.002` on the GPU,
  including the forced WENO subcell-limiter configuration.
- Python import-layer checks reported zero violations.

The float regression exposed cancellation in the WENO smoothness indicator.
The reconstruction now removes the constant component before evaluating that
quadratic form and clamps negative roundoff to zero. Device comparisons reject
non-finite states and eigenvalues explicitly.
