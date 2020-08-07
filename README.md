## Dependencies

Before you can compile our C++ code you should install the dependencies.

  - Asio 1.16.1 - (https://think-async.com/Asio/)
  - msgpack for C++ 3.3.0 (https://github.com/msgpack/msgpack-c)
  - Simple WebSocket 2.0.1 (https://gitlab.com/eidheim/Simple-WebSocket-Server)
  
These are attached to our code, you can find them in the `submodules` directory. They depend on the `OpenSSL` and `Boost` libraries, so you might want to install those as well.

Newer versions might work as well, but they are not guaranteed to be fully compatible with our code.

## Installation

To use the GDS C++ libraries in your application, you need the build and link the library with your code. The project uses a simple `Makefile` for building.

```sh
git clone https://github.com/arh-eu/gds-cpp-sdk.git
cd gds-cpp-sdk
make
```
This will create a folder named `output` containing two other folders - the `lib` folder inside will contain the static version for the GDS SDK (this `gdslib.a` file will be used for our console client and examples).

The `include` folder contains the header files which should be used. These use the standard C++ headers and the `<msgpack.hpp>` for the MessagePack structures.

Alternatively, you can compile the source files found in the `src` folder with a compiler that supports the `C++17` standard as well to create the library. You should not forget to link all dependencies with it, otherwise the compilation process will fail.

## Console Client

The Console Client gives you an easy way to communicate with the GDS without having to worry about programming your application.

It has built-in support for commonly used messages, like _events, attachments_ or _queries_.

### Compiling

To compile the Console Client, you should use the makefile in the `console_client` folder. This will create an executable named `gds_console_client.exe` that you can run.

```sh
cd console_client
make
```

### Options

To customize your client with the basic options, you should use the command line arguments.

#### URL

The URL of the GDS is given by the `-url` flag. The URL should be given _without_ the scheme. If not specified, `127.0.0.1:8888/gate` will be used (assuming a local GDS instance / simulator).

```sh
./gds_console_client.exe -url "192.168.111.222:8888/gate"
```

#### Username

The username can be specified with the `-username` flag. By default, `"user"` will be used.

```sh
./gds_console_client.exe -username "other_user"
```

#### Password

If you want to use password authentication, your password should be set with the `-password` flag.

```sh
./gds_console_client.exe -username "some_user" -password "my_password"
```

#### Timeout

The timeout of your requests can be set by the `-timeout` flag, in seconds. If you do not specify it, `30` will be used by default.

```sh
./gds_console_client.exe -timeout 10
```

#### HEX conversion

The `-hex` flag will convert the given string(s) to hexadecimal format. If you want to use multiple values, you should use semicolon (`;`) as separator in your string. The console client will print the values without connection.

```sh
./gds_console_client.exe -hex "picture1.png;picture2.bmp"
The hex value of 'picture1.png' is: 0x70696374757265312e706e67
The hex value of 'image2.bmp' is: 0x696d616765322e626d70
```


### Messages

#### INSERT, UPDATE, MERGE

The INSERT, UPDATE and MERGE messages are also known as EVENT messages. Events can have attachments as well, and you can upload these to the GDS by sending them with your event.

The event ID has to follow a format of "EVNTyyMMddHHmmssSSS0", where the first 4 letters are the abbreviation of "event", while the rest specifies a timestamp code from. This will make "EVNT2006241023125470" a valid ID in an event table.

The attachment ID has the same restriction, the difference is the prefix. Instead of the EVNT you should use ATID. The ID for the attachment can be "ATID2006241023125470".

Since the format is these messages have to follow is very strict, you will have to use hex values in your event strings for the binary IDs of your attachments. These hex values are unique identifiers for your binaries. To get the hex value of a string you can use the console client with the `-hex `flag to print these values. You can also enter multiple names, separating them by semicolon (`;`):

```sh
./gds_console_client.exe -hex "picture1.png;picture2.bmp"
The hex value of 'picture1.png' is: 0x70696374757265312e706e67
The hex value of 'image2.bmp' is: 0x696d616765322e626d70
```

These binary IDs (with the 0x prefix) have to be in your EVENT SQL string.

To attach files to your events (named "binary contents") you should use the `-attachments` flag with your EVENT.

About the hex:

The attachments you specify are stored in a different table in the GDS than the event's data (to increase performance, and one attachment might be used for multiple events). To create a connection between the two we have to reference the attachment ID in your event record. The attachment itself can have multiple fields connected to it (like meta descriptors). The binary part of the attachment usually cannot be inserted into a query easily, therefore a unique ID is used in the SQL string to resolve this issue. This is usually generated from the attachment's filename, but you can use any name you want. Because of how things are stored in the background we have to use hexadecimal format for these IDs (with the 0x prefix), thus it leads to converting the filename into a hex format (conversion can be done by the `-hex` option, see it above).

The binaries themselves are sent with the event data, in a map (associative array), where the keys are these IDs, and the values are the binary data themselves (represented as an `std::map<std::string, std::vector<std::uint8_t>>`).

These binaries are generated from the files you specify with the `-attachments` flag automatically by the client.

##### INSERT

```sh
./gds_console_client.exe -event "INSERT INTO multi_event (id, images) VALUES('EVNT2006241023125470', array('ATID2006241023125470')); INSERT INTO \"multi_event-@attachment\" (id, meta, data) VALUES('ATID2006241023125470', 'image/bmp', 0x70696374757265312e626d70 )" -attachments picture1.bmp
```

##### UPDATE
```sh
./gds_console_client.exe -event "UPDATE multi_event SET speed = 100 WHERE id = 'EVNT2006241023125470'"
```

##### MERGE
```sh
./gds_console_client.exe -event "MERGE INTO multi_event USING (SELECT 'TEST2006301005294810' as id, 'ABC123' as plate, 110 as speed) I ON (multi_event.id = I.id) WHEN MATCHED THEN UPDATE SET multi_event.speed = I.speed WHEN NOT MATCHED THEN INSERT (id, plate) VALUES (I.id, I.plate)"
```

#### ATTACHMENT request

To request an attachment you should use the `-attachment` option with the appropriate SELECT string.

```sh
./gds_console_client.exe -attachment "SELECT * FROM \"multi_event-@attachment\" WHERE id='ATID2006241023125470' and ownerid='EVNT2006241023125470' FOR UPDATE WAIT 86400"
```

Please be careful, as the table name in this examples should be in double quotes, so it should be escaped by backslash, otherwise the parameters will not be parsed right. The reason for this is that the SQL standard does not support hyphens in table names, therefore it should be treated differently.

The GDS might not have the specified attachment stored, in this case it can take a while until it can send you back its response. In this case your client will send back an ACK message which means that you have successfully received the attachments.

Your attachments will be saved in the attachments folder with the messageID as their file name.

#### QUERY

A simple SELECT query statement can be specified by the following command:

```sh
./gds_console_client.exe -query "SELECT * FROM multi_event"
```

The result will be saved to the `exports` folder, with the message ID as the filename.

It is possible, that your query has more than one pages available. By default, only 300 rows will be returned by the GDS (if you do not specify the LIMIT in your SQL and the config does not set another limit for this). In these cases you have send a Next Query Page request, which will give you the next 300 records found.

If you do not want to bother by manually sending these requests, you can use the -queryall flag instead, that will automatically send these Next Query Page requests as long as there are additional pages with records available.


## SDK usage

The code is separated into 3 different header files. The GDS Message types are declared in the `gds_types.hpp` file.
The core functions for communication can be found in the `gds_connection.hpp` header.

A `semaphore.hpp` is also added. This is not needed for user created applications, but our examples and console clients use them.

Please note that the usual `ws://` or `wss://` prefix is _not_ needed in the URL (it will lead to a connection refusal as the `SimpleWebSocketClient` expects the URL without the scheme).


### Creating the Client

You can obtain a pointer for the implementation object by calling the static `gds_lib::connection::GDSInterface::create(..)` method. This has two overloads depending on if you want to use TLS security or not. If not, the only url that should be passed is the GDS gate url (fe. `192.168.111.222:8888/gate`).

```cpp
std::shared_ptr<gds_lib::connection::GDSInterface> mGDSInterface = gds_lib::connection::GDSInterface::create("192.168.111.222:8888/gate");
```

### Callbacks

The client communicates with callbacks since it runs on a separate thread. You can specify your own callback function for the interface. It has four public members for this:
```cpp
struct GDSInterface {
  //...
  std::function<void()> on_open;
  std::function<void(gds_lib::gds_types::GdsMessage &)> on_message;
  std::function<void(int, const std::string&)> on_close;
  std::function<void(int, const std::string&)> on_error;
  //...
};
```

Your client code simply has to assign a value for these after you create the client:

```cpp
mGDSInterface->on_open    = [](){
	std::cout << "Client is open!" << std::endl;
};

mGDSInterface->on_close   = [](int status, const std::string& reason){
		std::cout << "Client closed: " << reason << " (code: " <<  code << ")" << std::endl;
};

mGDSInterface->on_error   = [](int code, const std::string& reason) {
	std::cout << "WebSocket returned error: " << reason << " (error code: " <<  code << ")" << std::endl;
};


mGDSInterface->on_message = [](gds_lib::gds_types::GdsMessage &msg) {
	std::cout << "I received a message!" << std::endl;
};
```

### Starting the client
To start your client you should simply invoke the `start()` method. This will initalize and create the WebSocket connection to the GDS.
Keep in mind that this does not send the login message, that is done by manually if you use the SDK.

```cpp
mGDSInterface->start();
```	

### Creating a message

To create a message you simply have to create an object of the type `GdsMessage`, found in the `gds_lib::gds_types` namespace. Messages have two parts, headers and data parts, you will read about how to create them below.

Many fields and/or members can have `null` values or skipped entirely. This is represented in our code with the `std::optional` wrapper.

For the list of the classes and member variables available, check the `gds_types.hpp` header.

```cpp
gds_lib::gds_types::GdsMessage fullMessage;
```

The structure of the GdsMessage is the following:

```cpp
struct GdsMessage : public Packable {
        std::string userName;
        std::string messageId;
        int64_t createTime;
        int64_t requestTime;
        bool isFragmented;
        std::optional<std::string> firstFragment;
        std::optional<std::string> lastFragment;
        std::optional<int32_t> offset;
        std::optional<int32_t> fds;
        int32_t dataType;
        std::shared_ptr<Packable> messageBody;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };
```

As you see the Message inherits from the `Packable` structure, which is a simple interface for messages that are sent to or by the GDS. These objects have two methods used for packing them via the MessagePack; the `pack(..)` puts them into a messagepack buffer while the `unpack(..)` initalizes them from a messagepack object.
Packable objects can be validated (the constraints on the objects should be checked here). For easier reading, these objects also have a string representation, that is obtained by the `to_string()` method, inherited from the `Stringable` interface.

```cpp
    struct Stringable {
        virtual std::string to_string() const { return {}; }
    };

    struct Packable : public Stringable {
        virtual ~Packable() {}
        virtual void pack(msgpack::packer<msgpack::sbuffer>&) const = 0;
        virtual void unpack(const msgpack::object&) = 0;
        virtual void validate() const {}
    };
```

#### Message Headers

The information in the header part can be set by simply assigning the values in the `GdsMessage` object.

```cpp
using namespace std::chrono;
long now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

fullMessage.username = "this_is_some_user";
fullMessage.createTime = now;
//...
```

#### Message Data

Since the data part can have many value, the `GdsMessage` class simply contains a (smart) pointer to a `Packable` object. The type itself determined by the `dataType` field in the header.

A `SELECT` query can be created by the following:


```cpp
std::shared_ptr<gds_lib::gds_types::GdsQueryRequestMessage> selectBody(new gds_lib::gds_types::GdsQueryRequestMessage());

selectBody->selectString = "SELECT * FROM multi_event";
selectBody->consistency = "PAGES";
selectBody->timeout = 0; //using GDS's default timeout

fullMessage.dataType = GdsMsgType::QUERY;
fullMessage.messageBody = selectBody;
```

With this the message is assigned with the proper type and the pointer to the memory location of containing the select data.

### Sending the message

Sending a message is simple, you should invoke the `send(..)` method on the GDS interface.

```cpp
mGDSInterface->send(fullMessage);
```

### Handling the reply

The message the GDS sends you is received by the GDSInterface, which will invoke the `on_message(..)` callback function (if) you have specified (it) at the initalization.

However, you want to have different logic based on the message type you received. This is done by checking the `dataType` field in the message.

```cpp
mGDSInterface->on_message = [](gds_lib::gds_types::GdsMessage &msg) {
{
    switch (msg.dataType) {
    case gds_types::GdsMsgType::LOGIN_REPLY: // Type 1
        //This is a login reply message.
        break;

    case gds_types::GdsMsgType::EVENT_REPLY: // Type 3
        //This is an event reply.
    	 break;
    //... rest of the cases, as neccessary.
    default:
    	break;
    }
}
```

The data part itself is simply represented as a `Packable` pointer, therefore you have to check its type and cast it before you can process it. Usually you want to have separate functions for these processings. These do not need to be lambda expressions, you can invoke regular functions obviously.

```cpp
auto handleLoginReply = [](
    GdsMessage& /*fullMessage*/, //if you need the full message as well you can pass it as well.
    std::shared_ptr<GdsLoginReplyMessage>& loginReply)
{
    if (loginReply->ackStatus == 200)
    {
        std::cout << "Login successful!" << std::endl;
    }
    else
    {
        std::cout << "Error during the login!" << std::endl;
    }
};

mGDSInterface->on_message = [](gds_lib::gds_types::GdsMessage &msg) {
{
    switch (msg.dataType) {
    case gds_types::GdsMsgType::LOGIN_REPLY: // Type 1
        //This is a login reply message.

        std::shared_ptr<GdsLoginReplyMessage> login_body = std::dynamic_pointer_cast<GdsLoginReplyMessage>(msg.messageBody);
        //process it as you wish. The ConsoleClient will invoke the following function:
        handleLoginReply(msg, body);
        break;
    }
}
```

### Closing the client

If you no longer need the client, you should invoke the `close()` method, which sends the standard close message for the WebSocket connection. The destructor also invokes this if it was not closed yet, however, you probably do not want to keep the connection open if it is not needed anymore.

```cpp
mGDSInterface->close();
```


### Implementation-defined behaviours

Since the size of the fundamental types (like `int` or `long`) is not specified by the standard but are left implementation-defined, you should avoid using these in messages, and use the exact sized-types like `std::int32_t` and `std::int64_t` found in the `<cstdint>` header (if present on your system).

Classes provided by our code works with these types, please be careful and use those in your applications as well, otherwise you might find yourself in an undefined-behaviour that cannot be tracked easily.

### Multi-threading

The SDK is written multi-threaded, meaning the WebSocket client runs on a separate thread - notifying you on the callbacks should anything happen.

## Examples

The `examples` folder has some running examples for the setup - you can use the server simulator for your needs (found [here](https://github.com/arh-eu/gds-server-simulator)). It's sequential by mean (by awaiting semaphores every time there's something to wait for), to make the communication and the replies clear and simplified, of course you will not need these in a live app.
