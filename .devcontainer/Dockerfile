FROM mcr.microsoft.com/devcontainers/cpp:ubuntu

ARG BAZELISK_VERSION=v1.22.1

RUN \
  apt-get update && \
  apt-get -q -y install \
    python python3 python3-pip \
    unzip tar gzip zip curl bash patch \
    libtinfo5 libncurses5 libncurses5-dev libncursesw5-dev

RUN pip install --upgrade pip

RUN curl -fsSL -o /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-amd64 && \
    chmod +x /usr/local/bin/bazel

RUN wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get install -q -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb

RUN apt-get update && \
    apt-get -q -y install libarrow-dev
