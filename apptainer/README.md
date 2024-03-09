# Apptainer

Here we maintain Apptainer Definition Files for different compilers.

## Build an Apptainer container

```shell
apptainer build <compiler>.sif Apptainer.<compiler>
```

Then copy the created .sif file to an HPC cluster.

## Build an Apptainer container using Docker

```shell
docker run -v ${PWD}:/work peanoframework/base apptainer build work/<compiler>.sif work/Apptainer.<compiler>
```

## Use an Apptainer container

```shell
apptainer shell -e --bind "$PWD" <compiler>.sif
```

In cases where `apptainer` is not available, you might want to try `singularity` instead.
