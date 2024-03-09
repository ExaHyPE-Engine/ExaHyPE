# Apptainer

Here we maintain Apptainer definition files for different compiler toolchains.

## Build an Apptainer container

```shell
apptainer build <toolchain>.sif Apptainer.<toolchain>
```

Then copy the created .sif file to an HPC cluster.

## Build an Apptainer container using Docker

```shell
docker run -v ${PWD}:/work peanoframework/base apptainer build work/<toolchain>.sif work/Apptainer.<toolchain>
```

## Use an Apptainer container

```shell
apptainer shell -e --bind "$PWD" <toolchain>.sif
```

In cases where `apptainer` is not available, you might want to try `singularity` instead.
