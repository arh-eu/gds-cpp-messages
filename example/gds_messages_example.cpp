#include "gds_messages_example.hpp"

#include <chrono>
#include <iostream>

using namespace gds_lib;
using namespace gds_lib::gds_types;

GDSMessageExamples::GDSMessageExamples() {

  callbacks.msgCallback = std::bind(&GDSMessageExamples::onMessageReceived,
                                    this, std::placeholders::_1);
  callbacks.msgErrorCallback = std::bind(&GDSMessageExamples::onInvalidMessage,
                                         this, std::placeholders::_1);
  callbacks.wsErrorCallback = std::bind(&GDSMessageExamples::onWebSocketError,
                                        this, std::placeholders::_1);
  callbacks.connReadyCallback = [this]() { this->connectionReady.notify(); };
  callbacks.wsClosedCallback = [this](const gds_types::WebsocketResult &) {
    this->connectionClosed.notify();
  };

  mGDSInterface = gds_lib::connection::GDSInterface::create(
      "127.0.0.1:8080/gate", callbacks, io_service.getIoService());
  // This also works:
  // mGDSInterface = gds_lib::connection::GDSInterface::create(
  //    "127.0.0.1", "8080", "gate", callbacks, io_service.getIoService());
}

GDSMessageExamples::~GDSMessageExamples() { close(); }

void GDSMessageExamples::close() {
  if (mGDSInterface) {
    mGDSInterface->close();
    mGDSInterface.reset();
  }
}

int64_t GDSMessageExamples::now() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

void GDSMessageExamples::run() {
  connectionReady.wait();
  std::cout << "WebSocket connection to GDS established!" << std::endl;

  login();
  nextMsgReceived.wait();

  event();
  nextMsgReceived.wait();

  attachmentRequest();
  nextMsgReceived.wait();

  eventDocument();
  nextMsgReceived.wait();

  selectFirst();
  nextMsgReceived.wait();

  selectNext();
  nextMsgReceived.wait();

  std::cout << "Closing WebSocket connection.." << std::endl;
  close();
  connectionClosed.wait();
  std::cout << "WebSocket connection to GDS closed!" << std::endl;
}

void GDSMessageExamples::login() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = "user";
  fullMessage.messageId = "";
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
    // loginBody->fragment_transmission_unit = std::nullopt;
    // loginBody->reserved_fields = std::nullopt;
  }
  fullMessage.messageBody = loginBody;

  std::cout << "Trying to send login message to the GDS.." << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}

void GDSMessageExamples::event() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = "user";
  fullMessage.messageId = "";
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::EVENT;

  std::shared_ptr<gds_lib::gds_types::GdsEventMessage> eventBody(
      new gds_lib::gds_types::GdsEventMessage());
  {
    eventBody->operations =
        "INSERT INTO table('col1', 'col2') VALUES('abc', 123)";
    eventBody->binaryContents["123"] = {0x14, 0x12, 0x5, 0x15};
  }
  fullMessage.messageBody = eventBody;

  std::cout << "Trying to send an event message to the GDS.." << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}

void GDSMessageExamples::attachmentRequest() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = "user";
  fullMessage.messageId = "";
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::ATTACHMENT_REQUEST;
  std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestMessage>
      attachmentReqBody(new gds_lib::gds_types::GdsAttachmentRequestMessage());
  {
    attachmentReqBody->request =
        "SELECT meta, data, \"@to_valid\" FROM \"events-@attachment\" WHERE id "
        "= 'ATID201811071434257890' and ownerid = 'EVNT201811020039071890' FOR "
        "UPDATE WAIT 86400";
  }
  fullMessage.messageBody = attachmentReqBody;

  std::cout << "Trying to send an attachment message to the GDS.." << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}

void GDSMessageExamples::eventDocument() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = "user";
  fullMessage.messageId = "";
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::EVENT_DOCUMENT;

  std::shared_ptr<gds_lib::gds_types::GdsEventDocumentMessage> eventBody(
      new gds_lib::gds_types::GdsEventDocumentMessage());
  {
    eventBody->tableName = "table";
    eventBody->fieldDescriptors.push_back({{"field1", "TEXT", "text/plain"}});
    eventBody->records.resize(1);
    GdsFieldValue value;
    value.type = msgpack::type::STR;
    value.value = std::string("some_value_for_field");
    eventBody->records[0].push_back(value);
  }
  fullMessage.messageBody = eventBody;

  std::cout << "Trying to send an event document message to the GDS.."
            << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}

void GDSMessageExamples::selectFirst() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = "user";
  fullMessage.messageId = "";
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::QUERY;

  std::shared_ptr<gds_lib::gds_types::GdsQueryRequestMessage> selectBody(
      new gds_lib::gds_types::GdsQueryRequestMessage());
  {
    selectBody->selectString = "SELECT * FROM table";
    selectBody->consistency = "PAGES";
    selectBody->timeout = 0;
    // selectBody->queryPageSize = std::nullopt;
    // selectBody->queryType = std::nullopt;
  }
  fullMessage.messageBody = selectBody;

  std::cout << "Trying to send first SELECT query message to the GDS.."
            << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}

void GDSMessageExamples::selectNext() {
  int64_t currentTime = now();

  gds_lib::gds_types::GdsMessage fullMessage;
  fullMessage.userName = "user";
  fullMessage.messageId = "";
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;
  fullMessage.dataType = gds_lib::gds_types::GdsMsgType::GET_NEXT_QUERY;

  std::shared_ptr<gds_lib::gds_types::GdsNextQueryRequestMessage> selectBody(
      new gds_lib::gds_types::GdsNextQueryRequestMessage());
  {
    selectBody->contextDescriptor =
        contextDescriptor; // we got it from the prev. SELECT
    selectBody->timeout = 0;
    // selectBody->queryPageSize = std::nullopt;
    // selectBody->queryType = std::nullopt;
  }
  fullMessage.messageBody = selectBody;

  std::cout << "Trying to send next SELECT query message to the GDS.."
            << std::endl;
  mGDSInterface->sendMessage(fullMessage);
}

void GDSMessageExamples::onMessageReceived(
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

void GDSMessageExamples::handleLoginReply(
    GdsMessage & /*fullMessage*/,
    std::shared_ptr<GdsLoginReplyMessage> &loginReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received a LOGIN message! ";
  std::cout << "Login reply status code is: " << loginReply->ackStatus
            << std::endl;
  nextMsgReceived.notify();
}

void GDSMessageExamples::handleEventReply(
    GdsMessage & /*fullMessage*/,
    std::shared_ptr<GdsEventReplyMessage> &eventReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received an EVENT reply message! ";
  std::cout << "Event reply status code is: " << eventReply->ackStatus
            << std::endl;
  nextMsgReceived.notify();
}

void GDSMessageExamples::handleEventDocumentReply(
    gds_lib::gds_types::GdsMessage &,
    std::shared_ptr<gds_lib::gds_types::GdsEventDocumentReplyMessage>
        &eventReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received an EVENT Document reply message! ";
  std::cout << "Event reply status code is: " << eventReply->ackStatus
            << std::endl;
  nextMsgReceived.notify();
}

void GDSMessageExamples::handleAttachmentRequestReply(
    gds_lib::gds_types::GdsMessage &,
    std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage>
        &replyBody) {
  std::cout << "CLIENT received an AttachmentRequestReply message! "
            << std::endl;
  std::cout << "Global status code is: " << replyBody->ackStatus << std::endl;
  nextMsgReceived.notify();
}

void GDSMessageExamples::handleQueryReply(
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
      std::cout << col.as<std::string>() << ' ';
      // the GDS simulator only sends strings.
      // otherwise the col.TYPE member can help us to call the 'as<..>()' with
      // the right template parameter type.
    }
    std::cout << ']' << std::endl;
    contextDescriptor = queryReply->response->queryContextDescriptor;
  }

  nextMsgReceived.notify();
}

void GDSMessageExamples::onInvalidMessage(const std::exception &exc) {

  std::cerr << "The structure of the message is invalid! Details: "
            << exc.what() << std::endl;
  nextMsgReceived.notify();
}

void GDSMessageExamples::onWebSocketError(
    const gds_types::WebsocketResult &result) {

  std::cerr << "WebSocket returned error code: " << result.status_code
            << ", message: " << result.error_message << std::endl;
}