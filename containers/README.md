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

You can find prebuilt Docker images for each compiler toolchain we support at [Dockerhub](https://hub.docker.com/u/peanoframework).
To run a specific toolchain Docker image you can execute the following command:

```shell
docker run -it -v ${PWD}:/work --rm --privileged peanoframework/<toolchain> /bin/bash
```

where `<toolchain>` needs to be replaced by the toolchain you want to use.

If you want to use JupyterLab, you can add port forwarding to the Docker command from above:

```shell
docker run -it -v ${PWD}:/work -p 53155:53155 --rm --privileged peanoframework/<toolchain> /bin/bash
```

In case you want to use GPUs, you need to amend the Docker command as follows (you need to have a GPU installed):

```shell
docker run -it -v ${PWD}:/work -p 53155:53155 --rm --gpus=all --privileged peanoframework/<toolchain> /bin/bash
```

When you run the Docker container interactively with `--gpus` using the command above,
you can quickly check whether GPU offloading is supported on your system by executing `XXX_stream`,
where `XXX` is the programming model.

## Apptainer (formerly known as Singularity)

Build an Apptainer container:

```shell
apptainer build <toolchain>.sif Apptainer.<toolchain>
```

Then copy the created .sif file to a HPC cluster.

Usage:

```shell
apptainer shell -e --bind "$PWD" <toolchain>.sif
```

In cases where `apptainer` is not available, you might want to try `singularity` instead.
