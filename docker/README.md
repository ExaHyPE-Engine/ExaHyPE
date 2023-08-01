# Dockerfiles

Build the Docker image via

```shell
docker build -t peanoframework/<toolchain> -f toolchains/Dockerfile.<toolchain> .
```

Login into the Dockerhub account you want to push to:

```shell
docker login
```

Push the Docker image to Dockerhub via

```shell
docker push peanoframework/<toolchain>
```
