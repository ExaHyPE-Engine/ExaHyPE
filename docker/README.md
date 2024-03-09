# Docker

Here we maintain Dockerfiles for different compilers.

## Build a Docker image

```shell
docker build -t peanoframework/<compiler> -f Dockerfile.<compiler> .
```

### Login into your Dockerhub account

```shell
docker login
```

### Push the Docker image to Dockerhub

```shell
docker push peanoframework/<compiler>
```

You can find prebuilt Docker images for each compiler we support at [Dockerhub](https://hub.docker.com/u/peanoframework).

## Run a Docker image

```shell
docker run -it -v ${PWD}:/work --rm --privileged peanoframework/<compiler> /bin/bash
```

where `<compiler>` needs to be replaced by the compiler you want to use.

### Run with GPU Support

```shell
docker run -it -v ${PWD}:/work --rm --gpus=all --privileged peanoframework/<compiler> /bin/bash
```

### Run with Port Forwarding

```shell
docker run -it -v ${PWD}:/work -p 53155:53155 --rm --privileged peanoframework/<compiler> /bin/bash
```
