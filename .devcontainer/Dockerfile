FROM ubuntu:rolling

RUN apt-get update  \
	&& apt-get install -y --no-install-recommends \
		apt-utils dialog git iproute2 procps lsb-release \
		build-essential \
		g++ \
		cmake \
		libboost-dev \
		valgrind  \
	&& apt-get autoremove -y \
	&& apt-get clean -y \
	&& rm -rf /var/lib/apt/lists/*

RUN mkdir -p /workspace/src /workspace/build
