FROM mcr.microsoft.com/devcontainers/cpp:ubuntu

ENV TZ=Europe/London
ENV LANG=en_GB.UTF-8
ENV LANGUAGE=en_GB.UTF-8
ENV LC_ALL=en_GB.UTF-8

ARG LLVM_VERSION=18
ARG BAZELISK_VERSION=v1.22.1

# Upgrade & Install
RUN \
  apt-get update && \
    apt-get -q -y install --no-install-recommends \
    time locales lsb-release software-properties-common \
    python python3 python3-pip \
    gnupg wget unzip tar gzip zip curl bash patch \
    libtinfo5 libncurses5 libncurses5-dev libncursesw5-dev

RUN pip install --upgrade pip


# Bazelisk install
RUN curl -fsSL -o /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-amd64 && \
  chmod +x /usr/local/bin/bazel


# Install Clang + LLVM
RUN \
  curl -fsSL -o /tmp/llvm.sh https://apt.llvm.org/llvm.sh && \
  chmod +x /tmp/llvm.sh && \
  /tmp/llvm.sh 18 all && \
  rm /tmp/llvm.sh

ENV PATH="/usr/lib/llvm-${LLVM_VERSION}/bin:/usr/lib/llvm-${LLVM_VERSION}/include:${PATH}"
ENV LD_LIBRARY_PATH="/usr/lib/llvm-${LLVM_VERSION}/lib:/usr/lib/x86_64-linux-gnu/"

RUN \
  update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 && \
  update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100


# Update tz + lang
RUN \
  ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
  echo "$TZ" > /etc/timezone && \
  dpkg-reconfigure tzdata

RUN echo locales locales/default_environment_locale select "en_GB.UTF-8" | debconf-set-selections
RUN echo locales locales/locales_to_be_generated select "en_GB.UTF-8 UTF-8" | debconf-set-selections

# Cleanup
RUN \
  apt-get autoremove -y && \
  apt-get clean autoclean && \
  rm -rf /var/lib/apt/lists/* \
  rm -rf /tmp/*
