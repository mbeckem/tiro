FROM ubuntu:rolling

RUN apt-get update && apt-get install -y \
	build-essential \
	g++ \
	cmake \
	libboost-dev

RUN mkdir -p /hammer/build
COPY . /hammer/src/
