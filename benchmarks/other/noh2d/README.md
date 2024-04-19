# Noh2D benchmark

This standalone benchmark calculates the Noh problem in 2D using SWIFT's minimal SPH scheme.

## Small setup

The small setup consists of a 100x100 particle grid and runs for 1000 timesteps. Data is dumped every 10th timestep.
On a modern desktop machine this benchmark takes ~5 seconds to run. Use this setup to verify the correctness of the implementation
by inspecting the output data in ParaView.

Compile and run using:

```bash
./build-small.sh && ./noh2d-small
```

Tweak setup parameters by editing the `setup-small.h` file.

## Big setup

The big setup consists of a 8000x8000 particle grid and runs for 10 timesteps. No data is dumped. 
On a modern desktop machine this benchmark takes ~30 seconds to run. Use this setup to benchmark the runtime of
individual sub-steps.

Compile and run using:

```bash
./build-big.sh && ./noh2d-big
```

Tweak setup parameters by editing the `setup-big.h` file.

## Troubleshooting

Currently, this benchmark only works with recent Clang/LLVM-based compilers.
Tweak the compiler used by editing the `-DCMAKE_CXX_COMPILER` flag in the build scripts.
