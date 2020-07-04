FROM ubuntu:rolling

RUN apt-get update  \
    && apt-get -y upgrade \
    && apt-get install -y --no-install-recommends \
    apt-utils git nano less \
    build-essential g++ cmake ninja-build 

WORKDIR /workspace
COPY . /workspace/
RUN mkdir /workspace/build

ARG CMAKE_BUILD_TYPE=Release
ARG TIRO_TESTS=1
ARG TIRO_WARNINGS=1
ARG TIRO_LTO=1
ARG TIRO_WERROR=0

RUN cd /workspace/build \
    && cmake .. \
    -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
    -DTIRO_TESTS=$TIRO_TESTS \
    -DTIRO_WARNINGS=$TIRO_WARNINGS \
    -DTIRO_WERROR=$TIRO_WERROR \
    -DTIRO_LTO=$TIRO_LTO \
    && cmake --build . -j $(nproc) \
    && bin/tiro_tests
