FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
	build-essential \
	bison \
	flex \
	git \
	make \
	&& rm -rf /var/lib/apt/lists/*
	
WORKDIR /app

COPY Code /app

RUN make
