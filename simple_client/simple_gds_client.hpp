#ifndef SIMPLE_GDS_CLIENT_HPP
#define SIMPLE_GDS_CLIENT_HPP


#include "gds_connection.hpp"
#include "gds_types.hpp"
#include "semaphore.hpp"

#include "arg_parse.hpp"

#include "io_service.hpp"

#include <cstdint>
#include <mutex>
#include <exception>

class SimpleGDSClient {
  std::shared_ptr<gds_lib::connection::GDSInterface> mGDSInterface;

  binary_semaphore_t loginReplySemaphore;
  binary_semaphore_t nextMsgReceived;

  binary_semaphore_t connectionReady;
  binary_semaphore_t connectionClosed;

  IoService                         io_service;
  gds_lib::connection::GDSCallbacks callbacks;
  ArgParser args;

  unsigned long timeout;


public:
  SimpleGDSClient(const ArgParser&);
  ~SimpleGDSClient();

  void run();
  void close();


private:

  void setupConnection();

  int64_t now();

  void login();
  void send_query(const std::string&);



  gds_lib::gds_types::QueryContextDescriptor contextDescriptor;
  void
  handleLoginReply(gds_lib::gds_types::GdsMessage &,
                   std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage> &);
  void
       handleEventReply(gds_lib::gds_types::GdsMessage &,
                        std::shared_ptr<gds_lib::gds_types::GdsEventReplyMessage> &);
  void handleAttachmentRequestReply(
      gds_lib::gds_types::GdsMessage &,
      std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage> &);
       void
       handleEventDocumentReply(gds_lib::gds_types::GdsMessage &,
                        std::shared_ptr<gds_lib::gds_types::GdsEventDocumentReplyMessage> &);
  void
  handleQueryReply(gds_lib::gds_types::GdsMessage &,
                   std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage> &);

  void onMessageReceived(gds_lib::gds_types::GdsMessage &);
  void onInvalidMessage(const std::exception &);
  void onWebSocketError(const gds_lib::gds_types::WebsocketResult &);
};

#endif
