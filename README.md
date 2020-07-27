## Compiling

To use it in your application, you need the build and link the libraries with your code. The code uses `cmake` for the building stages.

You can compile this with a compiler that supports the `C++17` standard:

```sh
make
```

## Dependencies

  - OpenSSL
  - Boost
  - Asio 1.16.1 - (https://think-async.com/Asio/)
  - msgpack for C++ 3.3.0 (https://github.com/msgpack/msgpack-c)
  - Simple WebSocket 2.0.1 (https://gitlab.com/eidheim/Simple-WebSocket-Server)
  
These are attached to our code, but please keep in mind that they have their own dependencies as well (like OpenSSL and Boost).

Newer versions might work as well, but its not guaranteed to be fully compatible with our code.


## Usage

The code is separated into 3 different header files. The GDS Message types are declared in the `gds_types.hpp` file.
The core functions for communication can be found in the `gds_connection.hpp` header. A `semaphore.hpp` is also added. This is not needed explicitly for the clients, but our examples use them.

You can obtain a pointer for the implementation object by calling the static `gds_lib::connection::GDSInterface::create(..)` method. This will require three parameters, the WebSocket URL as a string, an object containing your callbacks for any event (incoming message or WebSocket error), and a (shared) pointer for an asio service.

## Implementation-defined behaviours

Since the size of the fundamental types (like `int` or `long`) is not specified by the standard but are left implementation-defined, you should avoid using these, and use the exact sized-types like `int32_t` and `uint64_t` found in the `<cstdint>` header.

Classes provided by our code works with these types, please be careful and use those in your applications as well, otherwise you might find yourself in an undefined-behaviour that cannot be tracked easily.

## Multi-threading

This example is written multi-threaded, meaning the handling of the out- and incoming messages run on separate threads - notifying you on the callbacks should anything happen.

## Examples

The `examples` folder has some running examples for the setup - you can use the server simulator for your needs (found [here](https://github.com/arh-eu/gds-server-simulator)). It's sequential by mean (by awaiting semaphores every time there's something to wait for), to make the communication and the replies clear and simplified, of course you will not need these in a live app.
