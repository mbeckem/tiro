FROM ubuntu:rolling

RUN 	apt-get update && \
	apt-get install -y --no-install-recommends \
		build-essential \
		g++ \
		cmake \
		libboost-dev && \
	apt-get clean -y && \
	apt-get autoremove -y && \
	rm -rf /var/lib/apt/lists/*

RUN mkdir -p /hammer/build
COPY . /hammer/src/
