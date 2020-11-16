- [Dependencies](#dependencies)
- [Installation](#installation)
- [Console Client](#console-client)
  * [Compiling](#compiling)
  * [Options](#options)
    + [Help](#help)
    + [URL](#url)
    + [Username](#username)
    + [Password](#password)
    + [Cert, secret](#cert-secret)
    + [Timeout](#timeout)
    + [HEX conversion](#hex-conversion)
  * [Messages](#messages)
    + [INSERT, UPDATE, MERGE](#insert--update--merge)
      - [INSERT](#insert)
      - [UPDATE](#update)
      - [MERGE](#merge)
    + [ATTACHMENT request](#attachment-request)
    + [QUERY](#query)
- [SDK usage](#sdk-usage)
  * [Creating the Client](#creating-the-client)
  * [Callbacks](#callbacks)
  * [Starting the client](#starting-the-client)
  * [Creating a message](#creating-a-message)
    + [Message Headers](#message-headers)
    + [Message Data](#message-data)
  * [Sending the message](#sending-the-message)
  * [Handling the reply](#handling-the-reply)
  * [Creating / reading attachments](#creating---reading-attachments)
  * [Attachment requests / response](#attachment-requests---response)
  * [Saving / exporting attachments](#saving---exporting-attachments)
  * [Next Query pages](#next-query-pages)
  * [Saving messages](#saving-messages)
  * [Handling errors](#handling-errors)
  * [Field types](#field-types)
  * [Closing the client](#closing-the-client)
  * [Connection errors](#connection-errors)
  * [Implementation-defined behaviours](#implementation-defined-behaviours)
  * [Multi-threading](#multi-threading)


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
mkdir build
cd build
cmake ..
make
make install
```
This will create a folder named `output` in your project root folder, containing two other folders - the `lib` folder inside will contain the static version for the GDS SDK (this `libgds.a` file will be used for our console client).

The `include` folder contains the header files which should be used. These use the standard C++ headers and the `<msgpack.hpp>` for the MessagePack structures.

Alternatively, you can compile the source files found in the `src` folder with a compiler that supports the `C++17` standard as well to create the library. You should not forget to link all dependencies with it, otherwise the compilation process will fail.

## Console Client

The Console Client gives you an easy way to communicate with the GDS without having to worry about programming your application.

It has built-in support for commonly used messages, like _events, attachments_ or _queries_.

It is also useful if you want to create your own client but need some help or guidelines to start. In this case, simply check its source.

### Compiling

To compile the Console Client, you should use the makefile in the `console_client` folder. This will create an executable named `gds_console_client.exe` that you can run.

```sh
cd console_client
make
```

### Options

To customize your client with the basic options, you should use the command line arguments.

##### Help

If you need help about the usage, the syntax can be printed by the -help flag.

```shell
./gds_console_client.exe -help
```

#### URL

The URL of the GDS is given by the `-url` flag. The URL should be given _without_ the scheme. If not specified, `127.0.0.1:8888/gate` will be used (assuming a local GDS instance / simulator).

```sh
./gds_console_client.exe -url "192.168.111.222:8888/gate" -query "SELECT * FROM multi_event"
```

#### Username

The username can be specified with the `-username` flag. By default, `"user"` will be used.

```sh
./gds_console_client.exe -username "other_user" -query "SELECT * FROM multi_event"
```

#### Password

If you want to use password authentication, your password should be set with the `-password` flag.

```sh
./gds_console_client.exe -username "some_user" -password "my_password" -query "SELECT * FROM multi_event"
```

#### Cert, secret

The name of the file containing the certificate chain and your private key that should be used for secure (TLS) connection to the GDS should be given by the -cert option (it should be in PKCS12 format - a `*.p12`). The GDS uses different port (and endpoint) for default and for encrypted connection, therefore the url should be specified as well.

The password that was used to generate and encrypt the cert file should be given by the -secret flag.

```sh
./gds_console_client.exe -url "192.168.222.111:8443/gates" -cert "my_cert_file.p12" -secret "password_for_cert" -query "SELECT * FROM multi_event"
```

#### Timeout

The timeout of your requests can be set by the `-timeout` flag, in seconds. If you do not specify it, `3` will be used by default.

```sh
./gds_console_client.exe -timeout 10 -query "SELECT * FROM multi_event"
```

#### HEX conversion

The `-hex` flag will convert the given string(s) to hexadecimal format. If you want to use multiple values, you should use semicolon (`;`) as separator in your string. The console client will print the values without connection.

```sh
./gds_console_client.exe -hex "attachment_id_1.bmp;attachment_id_1.png"
The hex value of 'attachment_id_1.bmp' is: 0x6174746163686d656e745f69645f312e626d70
The hex value of 'attachment_id_1.png' is: 0x6174746163686d656e745f69645f312e706e67
```


### Messages

#### INSERT, UPDATE, MERGE

The INSERT, UPDATE and MERGE messages are also known as EVENT messages. Events can have attachments as well, and you can upload these to the GDS by sending them with your event.

The event ID has to follow a format of "EVNTyyMMddHHmmssSSS0", where the first 4 letters are the abbreviation of "event", while the rest specifies a timestamp code from. This will make "EVNT2006241023125470" a valid ID in an event table.

The attachment ID has the same restriction, the difference is the prefix. Instead of the EVNT you should use ATID. The ID for the attachment can be "ATID2006241023125470".

Since the format is these messages have to follow is very strict, you will have to use hex values in your event strings for the binary IDs of your attachments. These hex values are unique identifiers for your binaries. To get the hex value of a string you can use the console client with the `-hex `flag to print these values. You can also enter multiple names, separating them by semicolon (`;`):

```sh
./gds_console_client.exe -hex "attachment_id_1.bmp;attachment_id_1.png"
The hex value of 'attachment_id_1.bmp' is: 0x6174746163686d656e745f69645f312e626d70
The hex value of 'attachment_id_1.png' is: 0x6174746163686d656e745f69645f312e706e67
```

These binary IDs (with the 0x prefix) have to be in your EVENT SQL string.

To attach files to your events (named "binary contents") you should use the `-attachments` flag with your EVENT.

About the hex:

The attachments you specify are stored in a different table in the GDS than the event's data (to increase performance, and one attachment might be used for multiple events). To create a connection between the two we have to reference the attachment ID in your event record. The attachment itself can have multiple fields connected to it (like meta descriptors). The binary part of the attachment usually cannot be inserted into a query easily, therefore a unique ID is used in the SQL string to resolve this issue. This is usually generated from the attachment's filename, but you can use any name you want. Because of how things are stored in the background we have to use hexadecimal format for these IDs (with the 0x prefix), thus it leads to converting the filename into a hex format (conversion can be done by the `-hex` option, see it above).

The binaries themselves are sent with the event data, in a map (associative array), where the keys are these IDs, and the values are the binary data themselves (represented as an `std::map<std::string, std::vector<std::uint8_t>>`).

These binaries are generated from the files you specify with the `-attachments` flag automatically by the client.

##### INSERT

The following command assumes that there is a folder named 'attachments' next to the exe file with a file named `attachment_id_1.bmp`. 

```sh
./gds_console_client.exe -event "INSERT INTO multi_event (id, images) VALUES('EVNT2006241023125470', array('ATID2006241023125470')); INSERT INTO \"multi_event-@attachment\" (id, meta, data) VALUES('ATID2006241023125470', 'image/bmp', 0x6174746163686d656e745f69645f312e626d70 )" -attachments "attachment_id_1.bmp"
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

Your attachments will be saved in the attachments folder with the attachmentID as their file name.

#### QUERY

A simple SELECT query statement can be specified by the following command:

```sh
./gds_console_client.exe -query "SELECT * FROM multi_event"
```

The result will be saved to the `exports` folder, with the message ID as the filename.

It is possible, that your query has more than one pages available. By default, only 300 rows will be returned by the GDS (if you do not specify the LIMIT in your SQL and the config does not set another limit for this). In these cases you can use the -queryall flag instead, with that you will query all pages not just the first one.

## SDK usage

The code is separated into 3 different header files. The GDS Message types are declared in the `gds_types.hpp` file.
The core functions for communication can be found in the `gds_connection.hpp` header.

A `semaphore.hpp` is also added. This is not needed for user created applications, but our console client uses them. Another utility class is the `CountDownLatch`, which is used similar to a semaphore - also for the console client.

Please note that the usual `ws://` or `wss://` prefix is _not_ needed in the URL (it will lead to a connection refusal as the `SimpleWebSocketClient` expects the URL without the scheme, as a different constructor call indicates the TLS usage).

If you want to use `UUID`s for message ID, you can use the `gds_uuid.hpp` header, which has the `uuid::generate_uuid_v4();` method that returns a random `uuid` formatted string.

### Creating the Client

You can obtain a pointer for the implementation object through the `gds_lib::connection::GDSBuilder` class.

You can specify the desired username, password, timeout, URI and callbacks through this to make things easier. Since the messages are sent and received asnychronously, you also have to specify a shared pointer for the interface responsible to handle the incoming messages (see below).

```cpp
gds_lib::connection::GDSBuilder builder;
std::shared_ptr<gds_lib::connection::GDSInterface> client = builder
    .with_uri("192.168.111.222:8888/gate")
    .with_username("some_user")
    .with_callbacks(callbacks)
    .build();
```

If you want to use password authentication, you should pass the password argument too.

```cpp
gds_lib::connection::GDSBuilder builder;
std::shared_ptr<gds_lib::connection::GDSInterface> client = builder
    .with_uri("192.168.111.222:8888/gate")
    .with_username("some_user")
    .with_password("my_password")
    .with_callbacks(callbacks)
    .build();
```

To use TLS encryption, the PKCS12 file and the password used to decrypt it should be specified as well.

```cpp

class MyHandler : public gds_lib::connection::GDSMessageListener {
  //...
};

gds_lib::connection::GDSBuilder builder;
std::shared_ptr<MyHandler> callbacks = std::make_shared<MyHandler>();

std::shared_ptr<gds_lib::connection::GDSInterface> client = builder
    .with_uri("192.168.111.222:8888/gate")
    .with_username("some_user")
    .with_callbacks(callbacks)
    .with_tls(std::make_pair("cert_file.p12", "€3RT_P4$Sw0RĐ"))
    .build();
```


### Callbacks

The client communicates with callbacks since it runs on a separate thread. Your callback functions should be inherit the `GDSMessageListener` interface in the `gds_lib::connection` namespace. By default the implementation is the following:
```cpp
struct GDSMessageListener {
  virtual ~GDSMessageListener(){}

  //using gds_message_t = std::shared_ptr<gds_lib::gds_types::GdsMessage>;
  virtual void on_connection_success(gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>){}
  virtual void on_disconnect(){}
  virtual void on_connection_failure(const std::optional<gds_lib::connection::connection_error>&, std::optional<std::pair<gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>>>){
      throw not_implemented_error{"Connection failure handler was not overridden!", "GDSMessageListener::on_connection_failure()"};
  }

  virtual void on_event_ack3(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsEventReplyMessage> event_ack){
      throw not_implemented_error{"Received an Event ACK 3 but the method was not overridden!", "GDSMessageListener::on_event_ack3()"};
  }

  virtual void on_attachment_request4(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestMessage> request){
      throw not_implemented_error{"Received an Attachment Request 4 but the method was not overridden!", "GDSMessageListener::on_attachment_request4()"};
  }

  virtual void on_attachment_request_ack5(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage> request_ack){
      throw not_implemented_error{"Received an Attachment Request ACK 5 but the method was not overridden!", "GDSMessageListener::on_attachment_request_ack5()"};
  }

  virtual void on_attachment_response6(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage> response){
      throw not_implemented_error{"Received an Attachment Response 6 but the method was not overridden!", "GDSMessageListener::on_attachment_response6()"};
  }

 virtual void on_attachment_response_ack7(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseResultMessage> response_ack){
      throw not_implemented_error{"Received an Attachment Response ACK 7 but the method was not overridden!", "GDSMessageListener::on_attachment_response_ack7()"};
  }

  virtual void on_event_document8(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsEventDocumentMessage> event_document){
      throw not_implemented_error{"Received an Event Document 8 but the method was not overridden!", "GDSMessageListener::on_event_document8()"};
  }

  virtual void on_event_document_ack9(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsEventDocumentReplyMessage> event_document_ack){
      throw not_implemented_error{"Received an Event Document ACK 9 but the method was not overridden!", "GDSMessageListener::on_event_document_ack9()"};
  }

  virtual void on_query_request_ack11(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage> query_ack){
      throw not_implemented_error{"Received a Query Reply ACK 11 but the method was not overridden!", "GDSMessageListener::on_query_request_ack11()"};
  }
};
```

This means that if your methods are called without providing an override for the appropriate message type, they will throw an exception.

You have to pass a shared pointer for this object to the builder (see above).


### Starting the client
To start your client you should simply invoke the `start()` method. This will initialize and create the WebSocket connection to the GDS.
This method will automatically send the login message once the Websocket connection is open. If your login is successful, the `on_connection_success()` method will be invoked. Otherwise, if any error happens or the login process fails, you will be notified on the `on_connection_failure()`. This has two optional methods, because the error might come from an exception during the connection (timeout) or your login request could have been declined.

 You should not send any messages before the connection is successfully established, otherwise the GDS will drop your connection if the authentication process did not finish before your next message arrived.

```cpp
client->start();
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

As you see the Message inherits from the `Packable` structure, which is a simple interface for messages that are sent to or by the GDS. These objects have two methods used for packing them via the MessagePack; the `pack(..)` puts them into a messagepack buffer while the `unpack(..)` initializes them from a messagepack object.
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

fullMessage.userName = "this_is_some_user";
fullMessage.createTime = now;
//...
```

#### Message Data

Since the data part can have many value, the `GdsMessage` class simply contains a (smart) pointer to a `Packable` object. The type itself determined by the `dataType` field in the header.

A `SELECT` query can be created by the following:


```cpp
std::shared_ptr<gds_lib::gds_types::GdsQueryRequestMessage> selectBody = std::make_shared<gds_lib::gds_types::GdsQueryRequestMessage>();

selectBody->selectString = "SELECT * FROM multi_event";
selectBody->consistency = "PAGES";
selectBody->timeout = 0; //using GDS's default timeout

fullMessage.dataType = gds_lib::gds_types::GdsMsgType::QUERY;
fullMessage.messageBody = selectBody;
```

With this the message is assigned with the proper type and the pointer to the memory location of containing the select data.

### Sending the message

Sending a message is simple, you should invoke the `send(..)` method on the GDS interface.

```cpp
mGDSInterface->send(fullMessage);
```

### Handling the reply

The message the GDS sends you is received by the GDSInterface, which will invoke the specific `on_(..)` callback function in your listener.
```cpp
class MyHandler : public gds_lib::connection::GDSMessageListener {
  //rest of the methods

  virtual void GDSConsoleClient::on_query_request_ack11(gds_lib::gds_types::gds_message_t,
      std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage> queryReply) override
  {
      std::cout << "I received a SELECT reply message! The SELECT result is: \n" << queryReply->to_string() << std::endl;
  }

};
```

### Creating / reading attachments

You simply need to read a file and attach it as `std::vector<std::uint8_t>` to the messages. Do not forget that they should be stored with their hex IDs in the event map.

```cpp
  std::shared_ptr<gds_lib::gds_types::GdsEventMessage> eventBody = std::make_shared<gds_lib::gds_types::GdsEventMessage>();

	//some proper EVENT SQL string should be used based on what you want to insert/update.
  eventBody->operations = "";

  //this map is used to store the binaries
  std::map<std::string, gds_lib::gds_types::byte_array> binaries;

  //the file is opened as binary for reading.
  //you can seek its end automatically to get the number of bytes in it 
  //therefore making the vector insertion faster to avoid size duplication and moving during reading.
	
	std::fstream file(filename.c_str(), std::ios::binary | std::ios::in);
	std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  //the content is stored as a hex ID in the map from the filename
	std::stringstream ss;
	for (auto ch : filename) {
	    ss << std::hex << (int)ch;
	}
	std::string hex_filename = ss.str();

  binaries[hex_filename] = content; 

  //assigning the content to the event message
  eventBody->binaryContents = binaries;

  //send or add additional things if you would like
  client->send(fullMessage);
```

### Attachment requests / response

As defined in the [Attachment Request ACK Wiki](https://github.com/arh-eu/gds/wiki/Message-Data#attachment-request-ack---data-type-5), Attachment Request ACKs might not have the attachment in their body. In these cases you should await until the [Attachment Response](https://github.com/arh-eu/gds/wiki/Message-Data#attachment-response---data-type-6) is received with the binaries. 

Note however, that these messages should be ACKd with an [Attachment Response ACK](https://github.com/arh-eu/gds/wiki/Message-Data#attachment-response-ack---data-type-7). An example for this can be seen here:

```cpp
std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage> replyBody; //received from the GDS

gds_lib::gds_types::AttachmentResult result = replyBody->result;
//This result contains the attachment(s).
//The ACK should include what you received and if they got successfully stored.


gds_lib::gds_types::GdsMessage fullMessage;
//message headers set up as needed

fullMessage.dataType = gds_lib::gds_types::GdsMsgType::ATTACHMENT_REPLY;
std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseResultMessage> requestBody = std::make_shared<gds_lib::gds_types::GdsAttachmentResponseResultMessage>();
{
    requestBody->ackStatus = 200;
    requestBody->response = {};
    requestBody->response->status = 201;
    requestBody->response->result.requestIDs = result.requestIDs;
    requestBody->response->result.ownerTable = result.ownerTable;
    requestBody->response->result.attachmentID = result.attachmentID;
}

fullMessage.messageBody = requestBody;
client->send(fullMessage);

```

### Saving / exporting attachments

The attachments you receive are treated in the code as binary arrays, namely `std::vector<std::uint8_t>`.
You can simply save their content to get the attachment in your system to be open in an other application. There is also a type alias for this type as `byte_array` in the `gds_lib::gds_types` namespace.

```cpp
std::vector<std::uint8_t> binary_data;  // = result.attachment.value();

//the file should be opened for writing in binary mode.
//you can use std::ofstream as well if you like.
std::FILE* output = fopen("my_attachment", "wb");
fwrite(binary_data.data(), sizeof(std::uint8_t), binary_data.size(), output);
fclose(output);
```

### Next Query pages

The query message will query only the first page. If you want to query the next page, simply send a message of type 12 with the ContextDescriptor attached from the previous SELECT ACK.
```cpp
 std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage> queryReply; //obtained from the listener method as a parameter

if (queryReply->response) {
	//process the rest as needed.

	if (queryReply->response->hasMorePages) {

      gds_lib::gds_types::GdsMessage fullMessage;
      //setup as needed

	    fullMessage.dataType = gds_lib::gds_types::GdsMsgType::GET_NEXT_QUERY;
	    std::shared_ptr<gds_lib::gds_types::GdsNextQueryRequestMessage> selectBody = std::make_shared<gds_lib::gds_types::GdsNextQueryRequestMessage>();
	    {
	        selectBody->contextDescriptor = queryReply->response->queryContextDescriptor;
	        selectBody->timeout = 0;
	    }
	    fullMessage.messageBody = selectBody;

	    client->send(fullMessage);
    }
}
else {
    //this is usually some error in the select, check the error message.
}

```


### Saving messages

Every message has the `to_string()` method inherited through the `Packable : Stringable` classes.
You can use these to save the messages in a JSON-like format. Binary contents are represented as their size in these, so an image is displayed as `<2852 bytes>` instead of the raw bytes.

### Handling errors

The ACK messages have their status codes, which is available by the `ackStatus` field. The error message is in the `ackException` field. Since it might not be present (or set `null` by the GDS), this is an `std::optional<>` field as well.

### Field types

Fields sent by the GDS can have multiple types, seen in the [Wiki](https://github.com/arh-eu/gds/wiki/Message-Data#Message-field-descriptors).

This is represented internally as an `std::any<>` by the SDK. The fields themselves are of type `GdsFieldValue`, which structure is:

```cpp
struct GdsFieldValue : public Packable {
    std::any value;
    msgpack::type::object_type type;
    template <typename T>
    T as() const { return std::any_cast<T>(value); }
    std::string to_string() const override;

    void pack(msgpack::packer<msgpack::sbuffer>&) const override;
    void unpack(const msgpack::object&) override;
    void validate() const override;
};

```

The `type` can be used to indicate the original type which can be used to call the `as<T>` method to cast it to a proper value.

```cpp
GdsFieldValue obj;

  switch (obj.type) {
    case msgpack::type::NIL:
    {
    	//value is NULL.
    }
    break;
    case msgpack::type::BOOLEAN:
    {
    	bool value = obj.as<bool>();
    }
    break;
    case msgpack::type::POSITIVE_INTEGER:
    {
    	std::uint64_t value = obj.as<std::uint64_t>();
    }
    break;
    case msgpack::type::NEGATIVE_INTEGER:
    {

    	std::int64_t value = obj.as<std::int64_t>();
    }
    break;
    case msgpack::type::FLOAT32:
    {
		float value = obj.as<float>();
    }
    break;
    case msgpack::type::FLOAT64:
    {
    	double value = obj.as<double>();
    }
    break;
    case msgpack::type::STR:
    {
    	std::string value = obj.as<std::string>();
    }
    break;
    case msgpack::type::BIN:
    {
    	//using byte_array = std::vector<std::uint8_t>;
    	byte_array value = obj.as<byte_array>();
    }
    break;
    case msgpack::type::ARRAY:
    {
      std::vector<GdsFieldValue> value = obj.as<std::vector<GdsFieldValue>>();
      //...
      //first type should be parsed recursively if not empty.
      //then all can be casted simply with the `as<T>()` call.
    }
    break;
    case msgpack::type::MAP:
    {
    	std::map<std::string, std::string> value = obj.as<std::map<std::string, std::string>>();
    }
    break;
    default:
    	throw std::runtime_error("Unknown field value type");
  }

```

### Closing the client

If you no longer need the client, you should invoke the `close()` method, which sends the standard close message for the WebSocket connection. The destructor also invokes this if it was not closed yet, however, you probably do not want to keep the connection open if it is not needed anymore.

```cpp
client->close();
```

### Connection errors 

If the connection could not be established for some reason, your callback will be invoked with the reason, that can be either a status message received from the underlying WebSocket API or a login error.
The messages can be checked under the [Boost API's Core Error Codes](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference.html).

Since the error can come from two directions the two types of errors are represented by two `std::optional` values. If your connection failed because of some networking error, the `error` object will have its value filled, otherwise if the connection was fully established but the GDS declined your login request for some reason, the `reply` can be checked to retrieve what happened.

```cpp
void MyHandler::on_connection_failure(const std::optional<gds_lib::connection::connection_error>& error, 
        std::optional<std::pair<gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>>> reply) {
    //handle login errors 
}
```

### Implementation-defined behaviours

Since the size of the fundamental types (like `int` or `long`) are not specified by the standard but are left implementation-defined, this lib uses the types found in the `<cstdint>` header (like `std::int32_t` and `std::int64_t` for 32/64 bit integers) to ensure compatibility with other libs and reduce the chance of errors. 

### Multi-threading

The SDK is written multi-threaded, meaning the WebSocket client runs on a separate thread - notifying you on the callbacks should anything happen.
