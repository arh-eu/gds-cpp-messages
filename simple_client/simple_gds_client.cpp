#include "simple_gds_client.hpp"

#include <chrono>
#include <iostream>
#include "gds_uuid.hpp"

using namespace gds_lib;
using namespace gds_lib::gds_types;

SimpleGDSClient::SimpleGDSClient(const ArgParser& _args) : args(_args) {

  callbacks.msgCallback = std::bind(&SimpleGDSClient::onMessageReceived,
                                    this, std::placeholders::_1);
  callbacks.msgErrorCallback = std::bind(&SimpleGDSClient::onInvalidMessage,
                                         this, std::placeholders::_1);
  callbacks.wsErrorCallback = std::bind(&SimpleGDSClient::onWebSocketError,
                                        this, std::placeholders::_1);
  callbacks.connReadyCallback = [this]() { this->connectionReady.notify(); };
  callbacks.wsClosedCallback = [this](const gds_types::WebsocketResult &) {
    this->connectionClosed.notify();
  };

  timeout = std::stoul(args.get_arg("timeout")) * 1000;
  std::cout << "Timeout is set to " << timeout << "ms.." << std::endl;
  mGDSInterface = gds_lib::connection::GDSInterface::create(args.get_arg("url"), callbacks, io_service.getIoService());
  setupConnection();
}

SimpleGDSClient::~SimpleGDSClient() { close(); }

void SimpleGDSClient::close() {
  if (mGDSInterface) {
    mGDSInterface->close();
    mGDSInterface.reset();
  }
}

int64_t SimpleGDSClient::now() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

void SimpleGDSClient::setupConnection()
{
  std::cout << "Setting up WebSocket connection.." << std::endl;
  if(!connectionReady.wait_for(timeout))
  {
    throw new std::runtime_error("Timeout passed while waiting for connection setup!");
  }
  std::cout << "Sending login message.." << std::endl;
  login();
  std::cout << "Awaiting login ACK.." << std::endl;
  loginReplySemaphore.wait();
  std::cout << "Login ACK received!" << std::endl;
}

void SimpleGDSClient::run() {
    
  if(args.has_arg("query") || args.has_arg("queryall"))
  {
    std::string query_string = args.has_arg("query") ? args.get_arg("query") : args.get_arg("queryall");
    send_query(query_string);
  }

  if(!nextMsgReceived.wait_for(timeout))
  {
    throw new std::runtime_error("Timeout passed while waiting for the reply message!");
  }
}

void SimpleGDSClient::login() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = args.get_arg("username");
  fullMessage.messageId = uuid::generate_uuid_v4();
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::LOGIN;

  std::shared_ptr<gds_lib::gds_types::GdsLoginMessage> loginBody(
      new gds_lib::gds_types::GdsLoginMessage());
  {
    loginBody->serve_on_the_same_connection = false;
    loginBody->protocol_version_number = (2 << 16 | 9);
    loginBody->fragmentation_supported = false;
  }
  fullMessage.messageBody = loginBody;
  mGDSInterface->sendMessage(fullMessage);
}

void SimpleGDSClient::send_query(const std::string& query_str)
{
  
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = args.get_arg("username");
  fullMessage.messageId = uuid::generate_uuid_v4();
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::QUERY;

  std::shared_ptr<gds_lib::gds_types::GdsQueryRequestMessage> selectBody(
      new gds_lib::gds_types::GdsQueryRequestMessage());
  {
    selectBody->selectString = query_str;
    selectBody->consistency = "PAGES";
    selectBody->timeout = 0;
  }
  fullMessage.messageBody = selectBody;

  std::cout << "Trying to send SELECT query message to the GDS.." << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}



void SimpleGDSClient::onMessageReceived(
    gds_lib::gds_types::GdsMessage &msg) {
  switch (msg.dataType) {
  case gds_types::GdsMsgType::LOGIN_REPLY: // Type 1
  {
    std::shared_ptr<GdsLoginReplyMessage> body =
        std::dynamic_pointer_cast<GdsLoginReplyMessage>(msg.messageBody);
    handleLoginReply(msg, body);
  } break;
  case gds_types::GdsMsgType::EVENT_REPLY: // Type 3
  {
    std::shared_ptr<GdsEventReplyMessage> body =
        std::dynamic_pointer_cast<GdsEventReplyMessage>(msg.messageBody);
    handleEventReply(msg, body);
  } break;
  case gds_types::GdsMsgType::ATTACHMENT_REQUEST_REPLY: // Type 5
  {
    std::shared_ptr<GdsAttachmentRequestReplyMessage> body =
        std::dynamic_pointer_cast<GdsAttachmentRequestReplyMessage>(
            msg.messageBody);
    handleAttachmentRequestReply(msg, body);
  } break;
  case gds_types::GdsMsgType::ATTACHMENT_REPLY: // Type 7

    break;
  case gds_types::GdsMsgType::EVENT_DOCUMENT: // Type 8

    break;
  case gds_types::GdsMsgType::EVENT_DOCUMENT_REPLY: // Type 9
  {
    std::shared_ptr<GdsEventDocumentReplyMessage> body =
        std::dynamic_pointer_cast<GdsEventDocumentReplyMessage>(
            msg.messageBody);
    handleEventDocumentReply(msg, body);
  } break;
  case gds_types::GdsMsgType::QUERY_REPLY: // Type 11
  {
    std::shared_ptr<GdsQueryReplyMessage> body =
        std::dynamic_pointer_cast<GdsQueryReplyMessage>(msg.messageBody);
    handleQueryReply(msg, body);
  } break;
    break;
  default:

    break;
  }
}

void SimpleGDSClient::handleLoginReply(
    GdsMessage & /*fullMessage*/,
    std::shared_ptr<GdsLoginReplyMessage> &loginReply) {

    if(loginReply->ackStatus == 200){
      std::cout << "Login successful!" << std::endl;
    }else{
      std::cout <<"Error during the login!" << std::endl;
    }

    loginReplySemaphore.notify();
}

void SimpleGDSClient::handleEventReply(
    GdsMessage & /*fullMessage*/,
    std::shared_ptr<GdsEventReplyMessage> &eventReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received an EVENT reply message! ";
  std::cout << "Event reply status code is: " << eventReply->ackStatus
            << std::endl;
  nextMsgReceived.notify();
}

void SimpleGDSClient::handleEventDocumentReply(
    gds_lib::gds_types::GdsMessage &,
    std::shared_ptr<gds_lib::gds_types::GdsEventDocumentReplyMessage>
        &eventReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received an EVENT Document reply message! ";
  std::cout << "Event reply status code is: " << eventReply->ackStatus
            << std::endl;
  nextMsgReceived.notify();
}

void SimpleGDSClient::handleAttachmentRequestReply(
    gds_lib::gds_types::GdsMessage &,
    std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage>
        &replyBody) {
  std::cout << "CLIENT received an AttachmentRequestReply message! "
            << std::endl;
  std::cout << "Global status code is: " << replyBody->ackStatus << std::endl;
  nextMsgReceived.notify();
}

void SimpleGDSClient::handleQueryReply(
    GdsMessage & /*fullMessage*/,
    std::shared_ptr<GdsQueryReplyMessage> &queryReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received a SELECT reply message! ";
  std::cout << "SELECT status code is: " << queryReply->ackStatus << std::endl;

  if (queryReply->response) {

    std::cout << "The reply has: " << queryReply->response->numberOfHits
              << " rows." << std::endl;
    std::cout << "First row: [ ";
    std::vector<std::vector<GdsFieldValue>> &hits = queryReply->response->hits;

    for (auto &col : hits[0]) {
     std::cout << col.as_string() << ' ';
    }
    std::cout << ']' << std::endl;
    contextDescriptor = queryReply->response->queryContextDescriptor;
  }

  nextMsgReceived.notify();
}

void SimpleGDSClient::onInvalidMessage(const std::exception &exc) {

  std::cerr << "The structure of the message is invalid! Details: "
            << exc.what() << std::endl;
  nextMsgReceived.notify();
}

void SimpleGDSClient::onWebSocketError(
    const gds_types::WebsocketResult &result) {

  std::cerr << "WebSocket returned error code: " << result.status_code
            << ", message: " << result.error_message << std::endl;
}