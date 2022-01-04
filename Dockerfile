FROM debian:latest

RUN apt-get update

# GCC, CMake, Git and Boost, MC
RUN apt-get install -y g++ cmake git libboost-all-dev libssl-dev mc

# MSGPack install 

WORKDIR /home
RUN git clone https://github.com/msgpack/msgpack-c.git
WORKDIR msgpack-c
RUN git checkout cpp_master
RUN cmake -DMSGPACK_CXX14=ON .
RUN cmake --build . --target install

# WebSocket install

WORKDIR /home
RUN git clone https://gitlab.com/eidheim/Simple-WebSocket-Server.git
WORKDIR Simple-WebSocket-Server
RUN mkdir build
WORKDIR build
RUN cmake ..
RUN make
RUN make install

# GDS C++ SDK

WORKDIR /home
RUN git clone https://github.com/arh-eu/gds-cpp-sdk.git
WORKDIR gds-cpp-sdk
RUN mkdir build
WORKDIR build
RUN cmake ..
RUN make
RUN make install