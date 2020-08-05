#include "console_client.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <utility>
#include <vector>

#include "gds_uuid.hpp"

using namespace gds_lib;
using namespace gds_lib::gds_types;

SimpleGDSClient::SimpleGDSClient(const ArgParser& _args) : args(_args), connection_open(false) {

  timeout = std::stoul(args.get_arg("timeout")) * 1000;
  std::cout << "Timeout is set to " << timeout << "ms." << std::endl;
  std::cout << "Setting up ConsoleClient.." << std::endl;

  mGDSInterface = gds_lib::connection::GDSInterface::create(args.get_arg("url"));

  mGDSInterface->on_open    = std::bind(&SimpleGDSClient::onOpen, this);
  mGDSInterface->on_close   = std::bind(&SimpleGDSClient::onClose, this, std::placeholders::_1, std::placeholders::_2);
  mGDSInterface->on_message = std::bind(&SimpleGDSClient::onMessageReceived, this, std::placeholders::_1);
  mGDSInterface->on_error   = std::bind(&SimpleGDSClient::onError, this, std::placeholders::_1, std::placeholders::_2);

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
  std::cout << "Initializing WebSocket connection.." << std::endl;
  if(!connectionReady.wait_for(timeout))
  {
    throw new std::runtime_error("Timeout passed while waiting for connection setup!");
  }

  if(connection_open.load())
  {
    std::cout << "WebSocket connection with GDS successful!" << std::endl;
    std::cout << "Sending login message.." << std::endl;
    login();
    std::cout << "Awaiting login ACK.." << std::endl;
    loginReplySemaphore.wait();
    std::cout << "Login ACK received!" << std::endl;
  }
}

void SimpleGDSClient::run() {
  if(connection_open.load())
  {
    if(args.has_arg("query") || args.has_arg("queryall"))
    {
      std::string query_string = args.has_arg("query") ? args.get_arg("query") : args.get_arg("queryall");
      query_all = args.has_arg("queryall");
      send_query(query_string);
/*
      if(!nextMsgReceived.wait_for(timeout))
      {
        throw new std::runtime_error("Timeout passed while waiting for the query reply message!");
      }
      */
    }
    else if (args.has_arg("event"))
    {
      std::string event_string = args.get_arg("event");
      std::string file_list;
      if(args.has_arg("files")){
        file_list = args.get_arg("files");
      }
      send_event(event_string, file_list);
    }
    else if(args.has_arg("attachment"))
    {
      std::string attach_string = args.get_arg("attachment");
      send_attachment_request(attach_string);
    }

    workDone.wait();
  }
}

void SimpleGDSClient::login() {
  GdsMessage fullMessage = create_default_message();

  fullMessage.dataType = GdsMsgType::LOGIN;

  std::shared_ptr<GdsLoginMessage> loginBody(new GdsLoginMessage());
  {
    loginBody->serve_on_the_same_connection = false;
    loginBody->protocol_version_number = (2 << 16 | 9);
    loginBody->fragmentation_supported = false;
  }
  fullMessage.messageBody = loginBody;
  mGDSInterface->send(fullMessage);
}

void SimpleGDSClient::send_query(const std::string& query_str)
{
  GdsMessage fullMessage = create_default_message();
  message_id = fullMessage.messageId;

  fullMessage.dataType = GdsMsgType::QUERY;

  std::shared_ptr<GdsQueryRequestMessage> selectBody(new GdsQueryRequestMessage());
  {
    selectBody->selectString = query_str;
    selectBody->consistency = "PAGES";
    selectBody->timeout = 0;
  }
  fullMessage.messageBody = selectBody;

  std::cout << "Trying to send SELECT query message to the GDS.." << std::endl;
  mGDSInterface->send(fullMessage);
}


void SimpleGDSClient::send_next_query()
{
  GdsMessage fullMessage = create_default_message();
  fullMessage.messageId = message_id;

  fullMessage.dataType = GdsMsgType::GET_NEXT_QUERY;

  std::shared_ptr<GdsNextQueryRequestMessage> selectBody(new GdsNextQueryRequestMessage());
  {
    selectBody->contextDescriptor = contextDescriptor;
    selectBody->timeout = 0;
  }
  fullMessage.messageBody = selectBody;

  std::cout << "Trying to send next SELECT query message to the GDS.." << std::endl;
  mGDSInterface->send(fullMessage);
}


void SimpleGDSClient::send_event(const std::string& event_str, const std::string& file_list)
{

  GdsMessage fullMessage = create_default_message(); 
  fullMessage.dataType = GdsMsgType::EVENT;

  std::shared_ptr<GdsEventMessage> eventBody(new GdsEventMessage());
  {
    eventBody->operations = event_str;
    if(file_list.length() != 0){
      std::map<std::string, byte_array> binaries;

      std::stringstream file_list_stream(file_list);
      std::string tmp;
      std::vector<std::string> filenames;
      constexpr static char delimiter = ';';
      while(std::getline(file_list_stream, tmp, delimiter))
      {
        filenames.emplace_back(tmp);
      }

      for(const auto& filename : filenames)
      {
        std::string filepath = "attachments/";
        filepath += filename;

        std::basic_ifstream<std::uint8_t> file(filepath, std::ios::binary|std::ios::ate);
        if(file.is_open())
        {
          auto pos = file.tellg();

          byte_array content;
          content.reserve(pos);
          std::cout << "Reserved " << pos << " bytes" << std::endl;

          file.seekg(0, std::ios::beg);
          content.resize(pos);
          file.read(content.data(), pos);

          binaries[SimpleGDSClient::to_hex(filename)] = content;
          std::cout << "Adding " << filename << " as an attachment.." << std::endl;
        }
        else
        {
          std::cout << "Could not open the attachment file named '" << filename <<"'!";
        }
      }

      eventBody->binaryContents = binaries;
    }
  }
  fullMessage.messageBody = eventBody;
  std::cout << "Sending the EVENT message to the GDS.." << std::endl;
  mGDSInterface->send(fullMessage);
}


void SimpleGDSClient::send_attachment_request(const std::string& request)
{
  GdsMessage fullMessage = create_default_message();
  message_id = fullMessage.messageId;

  fullMessage.dataType = GdsMsgType::ATTACHMENT_REQUEST;

  std::shared_ptr<GdsAttachmentRequestMessage> requestBody(new GdsAttachmentRequestMessage());
  {
    requestBody->request = request;
  }
  fullMessage.messageBody = requestBody;

  std::cout << "Trying to send attachment request message to the GDS.." << std::endl;
  mGDSInterface->send(fullMessage);
}

gds_lib::gds_types::GdsMessage SimpleGDSClient::create_default_message()
{
  int64_t currentTime = now();
  gds_lib::gds_types::GdsMessage fullMessage;

  fullMessage.userName = args.get_arg("username");
  fullMessage.messageId = uuid::generate_uuid_v4();
  fullMessage.createTime = currentTime;
  fullMessage.requestTime = currentTime;
  fullMessage.isFragmented = false;

  return fullMessage;
}


void SimpleGDSClient::onMessageReceived(
  gds_lib::gds_types::GdsMessage &msg) {

  save_message(msg);
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
  case gds_types::GdsMsgType::ATTACHMENT: // Type 6
  {
    std::shared_ptr<GdsAttachmentResponseMessage> body =
    std::dynamic_pointer_cast<GdsAttachmentResponseMessage>(
      msg.messageBody);
    handleAttachmentResponse(msg, body);
  } break;
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


std::string SimpleGDSClient::to_hex(const std::string& src)
{
  std::stringstream ss;
  for(auto ch : src)
  {
    ss << std::hex << (int)ch;
  }
  return ss.str();
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
    GdsMessage & fullMessage,
  std::shared_ptr<GdsEventReplyMessage> &eventReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received an EVENT reply message! ";
  std::cout << "Event reply status code is: " << eventReply->ackStatus
  << std::endl;

  std::cout << "Full message:" << std::endl;
  std::cout << fullMessage.to_string() << std::endl;
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

  if(replyBody->ackStatus == 200)
  {
    AttachmentRequestBody& body = replyBody->request.value();
    AttachmentResult& result = body.result;
    if(result.attachment)
    {
      std::cout << "Attachment received as well, saving.." << std::endl;
      std::vector<uint8_t>& binary_data = result.attachment.value();
      std::string filename = "attachments/";
      filename += result.attachmentID;

      std::string extension = ".unknown";
      if(result.meta)
      {
        std::string mimetype = result.meta.value();
        if(mimetype == "image/png"){
          extension = ".png";
        }
        else if(mimetype == "image/jpg" || mimetype == "image/jpeg"){
          extension = ".jpg";
        }
        else if(mimetype == "image/bmp"){
          extension = ".bmp";
        }
        else if(mimetype == "video/mp4"){
          extension = ".mp4";
        }
      }
      filename += extension;

      FILE* output = fopen(filename.c_str(), "wb");
      if(output){
        fwrite(binary_data.data(), sizeof(std::uint8_t), binary_data.size(), output);
        fclose(output);
      }
      else
      {
        std::cout << "Could not open " + filename + "!" << std::endl;
      }

      workDone.notify();
    }
    else
    {
      std::cout << "Attachment not yet received, awaiting.." << std::endl;
    }
  }
  else
  {
    if(replyBody->ackException)
    {
      std::cout << replyBody->ackException.value() << std::endl;
    }
    workDone.notify();
  }

  nextMsgReceived.notify();
}


void SimpleGDSClient::handleAttachmentResponse(
  gds_lib::gds_types::GdsMessage &,
  std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage>
  &replyBody) {
  std::cout << "CLIENT received an AttachmentResponse message! "
  << std::endl;

      AttachmentResult result = replyBody->result;
    
      std::cout << "Saving attachment.." << std::endl;
      std::vector<uint8_t>& binary_data = result.attachment.value();
      std::string filename = "attachments/";
      filename += result.attachmentID;

      std::string extension = ".unknown";
      if(result.meta)
      {
        std::string mimetype = result.meta.value();
        if(mimetype == "image/png"){
          extension = ".png";
        }
        else if(mimetype == "image/jpg" || mimetype == "image/jpeg"){
          extension = ".jpg";
        }
        else if(mimetype == "image/bmp"){
          extension = ".bmp";
        }
        else if(mimetype == "video/mp4"){
          extension = ".mp4";
        }
      }
      filename += extension;

      FILE* output = fopen(filename.c_str(), "wb");
      if(output){
        fwrite(binary_data.data(), sizeof(std::uint8_t), binary_data.size(), output);
        fclose(output);
      }
      else
      {
        std::cout << "Could not open " + filename + "!" << std::endl;
      }

      {
        GdsMessage fullMessage = create_default_message();
        message_id = fullMessage.messageId;

        fullMessage.dataType = GdsMsgType::ATTACHMENT_REPLY;
        std::shared_ptr<GdsAttachmentResponseResultMessage> requestBody(new GdsAttachmentResponseResultMessage());
        {
          requestBody-> ackStatus = 200;
          requestBody->response = {};
          requestBody->response->status = 201;
          requestBody->response->result.requestIDs = result.requestIDs;
          requestBody->response->result.ownerTable = result.ownerTable;
          requestBody->response->result.attachmentID = result.attachmentID;
          
        }
        fullMessage.messageBody = requestBody;

        std::cout << "Send attachment response result message to the GDS.." << std::endl;
        mGDSInterface->send(fullMessage);
      }

    workDone.notify();
    nextMsgReceived.notify();
}

void SimpleGDSClient::handleQueryReply(
  GdsMessage & fullMessage,
  std::shared_ptr<GdsQueryReplyMessage> &queryReply) {
  // do whatever you want to do with this.

  std::cout << "CLIENT received a SELECT reply message! ";
  std::cout << "SELECT status code is: " << queryReply->ackStatus << std::endl;

  bool hasMorePages = false;
  if (queryReply->response) {
    hasMorePages = queryReply->response->hasMorePages;
    contextDescriptor = queryReply->response->queryContextDescriptor;
  }

  nextMsgReceived.notify();

  if(query_all && hasMorePages){
    send_next_query();
  }else{
    workDone.notify();
  }
}


void SimpleGDSClient::save_message(gds_lib::gds_types::GdsMessage &fullMessage)
{
  std::string  filename = "exports/";
  filename += fullMessage.messageId;
  filename += ".txt";
  std::ofstream output(filename);
  if(output.is_open())
  {
    std::cout << "Full message is saved as " << filename << std::endl;
    output << fullMessage.to_string();
  }
  else
  {
    std::cout << "Could not open " + filename + "!" << std::endl;
  }
}

void SimpleGDSClient::onError(int code, const std::string& reason) {
  std::cerr << "WebSocket returned error: " << reason << " (error code: " <<  code << ")" << std::endl;
  if(code == 111 || code == 2){
    connection_open.store(false);
    connectionReady.notify(); 
    workDone.notify();
  }
}

void SimpleGDSClient::onOpen() {
  connection_open.store(true);
  connectionReady.notify(); 
}

void SimpleGDSClient::onClose(int code, const std::string& reason) {
  std::cerr << "WebSocket closed: " << reason << " (code: " <<  code << ")" << std::endl;
  connection_open.store(false);
  connectionClosed.notify();
  workDone.notify();
}