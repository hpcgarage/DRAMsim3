FROM ubuntu:20.04

WORKDIR ../DRAMSIM3

RUN  apt-get update \
  && apt-get install -y make \
  && apt-get install -y cmake \
  && apt-get install -y build-essential