FROM ubuntu:rolling

RUN apt-get update  \
    && apt-get -y upgrade \
    && apt-get install -y --no-install-recommends \
        apt-utils git nano less \
        build-essential g++ cmake ninja-build \
        libboost-dev

WORKDIR /workspace
COPY . /workspace
RUN mkdir /workspace/build

ENV BUILD_MODE=Release \
    TIRO_TESTS=1 \
    TIRO_WARNINGS=1 \
    TIRO_LTO=1 \
    TIRO_WERROR=0

CMD cd /workspace/build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_MODE \
        -DTIRO_TESTS=$TIRO_TESTS \
        -DTIRO_WARNINGS=$TIRO_WARNINGS \
        -DTIRO_WERROR=$TIRO_WERROR \
        -DTIRO_LTO=$TIRO_LTO \
    && cmake --build . -j $(nproc) \
    && bin/tiro_tests
