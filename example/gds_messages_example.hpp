#ifndef GDS_MESSAGES_EXAMPLE_HPP
#define GDS_MESSAGES_EXAMPLE_HPP

#include "gds_connection.hpp"
#include "gds_types.hpp"
#include "semaphore.hpp"

#include "io_service.hpp"

#include <cstdint>
#include <mutex>

class GDSMessageExamples {
  std::shared_ptr<gds_lib::connection::GDSInterface> mGDSInterface;

  binary_semaphore_t nextMsgReceived;

  binary_semaphore_t connectionReady;
  binary_semaphore_t connectionClosed;

  IoService                         io_service;
  gds_lib::connection::GDSCallbacks callbacks;

public:
  GDSMessageExamples();
  ~GDSMessageExamples();

  void run();

private:
  int64_t now();
  void    close();

  // examples:
  void login();             // type 0
  void event();             // type 2
  void attachmentRequest(); // type 4
  void eventDocument();     // type 8
  void selectFirst();       // type 10
  void selectNext();        // type 12

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

// GDS -> client
// valid:
//  1
//  3
//  5
//  7
//  8
//  9
// 11

// invalid:
//  0
//  2
// 10
// 12
// 13
// 14

// client -> GDS
// valid
//  0
//  2
//  4
//  5
//  6
//  7
//  8
//  9
// 10
// 12

// invalid:
//  1
//  3
// 11
// 13
// 14

#endif // GDS_MESSAGES_EXAMPLE_HPP