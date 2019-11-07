from ubuntu:18.04

run apt-get update; \
    apt-get upgrade -y; \
    apt-get install -y qemu g++ g++-multilib make git

WORKDIR /work

cmd make clean test

