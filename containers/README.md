# Containers
## Spack
We use Spack as central package manager for installing software and dependencies.

## Docker
We maintain Dockerfiles for different compiler toolchains.

Build the Docker image:

```shell
docker build -t peanoframework/<toolchain> -f Dockerfile.<toolchain> .
```

Login into your Dockerhub account:

```shell
docker login
```

Push the Docker image to Dockerhub:

```shell
docker push peanoframework/<toolchain>
```

## Apptainer (formerly known as Singularity)

Build an Apptainer container:

```shell
apptainer build <toolchain>.sif Apptainer.<toolchain>
```

Then copy the resulting .sif to a HPC cluster.

Usage:

```shell
apptainer shell -e --bind "$PWD" <toolchain>.sif
```

In cases where `apptainer` is not available, you might want to try `singularity` instead.
