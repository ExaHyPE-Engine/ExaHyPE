# Docker

Here we maintain Dockerfiles for different compiler toolchains.

## Build a Docker image

```shell
docker build -t peanoframework/<toolchain> -f Dockerfile.<toolchain> .
```

### Login into your Dockerhub account

```shell
docker login
```

### Push the Docker image to Dockerhub

```shell
docker push peanoframework/<toolchain>
```

You can find prebuilt Docker images for each compiler toolchain we support at [Dockerhub](https://hub.docker.com/u/peanoframework).

## Run a Docker image

```shell
docker run -it -v ${PWD}:/work --rm --privileged peanoframework/<toolchain> /bin/bash
```

where `<toolchain>` needs to be replaced by the toolchain you want to use.

### Run with GPU Support

```shell
docker run -it -v ${PWD}:/work --rm --gpus=all --privileged peanoframework/<toolchain> /bin/bash
```

### Run with Port Forwarding

```shell
docker run -it -v ${PWD}:/work -p 53155:53155 --rm --privileged peanoframework/<toolchain> /bin/bash
```
