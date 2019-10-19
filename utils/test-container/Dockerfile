FROM ubuntu:rolling

RUN apt-get update  \
	&& apt-get -y upgrade \
	&& apt-get install -y --no-install-recommends \
		apt-utils git nano build-essential g++ cmake libboost-dev \
	&& apt-get autoremove -y \
	&& apt-get clean -y \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace
RUN mkdir /workspace/build

CMD cd /workspace/build \
    && cmake .. -DCMAKE_BUILD_TYPE=Debug -DHAMMER_TESTS=1 -DHAMMER_WARNINGS=1 -DHAMMER_WERROR=1 \
    && make -j$(nproc) \
    && test/unit_tests

