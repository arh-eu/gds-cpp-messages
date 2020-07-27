The submodules present here have the following versions:

  - Asio 1.16.1 - (https://think-async.com/Asio/)
  - msgpack for C++ 3.3.0 (https://github.com/msgpack/msgpack-c)
  - Simple WebSocket 2.0.1 (https://gitlab.com/eidheim/Simple-WebSocket-Server)

No changes were made to these, you might even use a newer version if present. However, compatibility is not guaranteed if there were any major changes done.

# Installation

Since WebSocket needs asio to work, it should be installed after asio.

## asio

```shell
unzip asio-1.16.1
cd asio-1.16.1
sudo ./configure
```

## msgpack

```shell
unzip msgpack-c-cpp-3.3.0.zip
cd msgpack-c-cpp-3.3.0
cmake -DMSGPACK_C17=ON .
sudo make install
```

## WebSocket

```shell
unzip Simple-WebSocket-Server-v2.0.1
cd Simple-WebSocket-Server-v2.0.1
mkdir build
cd build
cmake ..
sudo make install
```


For additional details about the installation please see their README or INSTALL files.