# Dockerfiles

Build the Docker image via

```shell
docker build -t peanoframework/<toolchain> -f Dockerfile.<toolchain> .
```

Login into your Dockerhub account:

```shell
docker login
```

Push the Docker image to Dockerhub via

```shell
docker push peanoframework/<toolchain>
```
