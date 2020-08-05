#ifndef CONSOLE_CLIENT_HPP
#define CONSOLE_CLIENT_HPP


#include "gds_connection.hpp"
#include "gds_types.hpp"
#include "semaphore.hpp"

#include "arg_parse.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <exception>

class SimpleGDSClient {
  std::shared_ptr<gds_lib::connection::GDSInterface> mGDSInterface;

  binary_semaphore_t loginReplySemaphore;
  binary_semaphore_t nextMsgReceived;

  binary_semaphore_t connectionReady;
  binary_semaphore_t connectionClosed;

  ArgParser args;

  unsigned long timeout;
  std::atomic_bool connection_open;

public:
  SimpleGDSClient(const ArgParser&);
  ~SimpleGDSClient();

  void run();
  void close();
  static std::string to_hex(const std::string&);


private:

  void setupConnection();

  int64_t now();

  void login();
  void send_query(const std::string&);
  void send_event(const std::string&, const std::string&);


  gds_lib::gds_types::GdsMessage create_default_message();

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

  void onOpen();
  void onClose(int, const std::string&);
  void onMessageReceived(gds_lib::gds_types::GdsMessage &);
  void onError(int, const std::string&);
};

#endif //CONSOLE_CLIENT_HPP
