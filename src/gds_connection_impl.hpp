#ifndef GDS_CONNECTION_IMPL_HPP
#define GDS_CONNECTION_IMPL_HPP

#include "gds_connection.hpp"
#include "gds_safe_queue.hpp"
#include "semaphore.hpp"

#include "client_ws.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace gds_lib {
namespace connection {

using ws_client_t = SimpleWeb::SocketClient<SimpleWeb::WS>;
using ws_connection_sptr = std::shared_ptr<ws_client_t::Connection>;

class GDSConnection : public GDSInterface {
private:
  const GDSCallbacks mGDSCallback;
  ws_client_t        mWebSocket;

  ws_connection_sptr mConnection;
  std::atomic_bool   mCloseRequested;

  std::atomic_bool   mClosed;

  gds_lib::SafeQueue<std::string> mInMessages;
  binary_semaphore_t              mInSemaphore;
  std::thread                     mInMessageThread;

  gds_lib::SafeQueue<gds_lib::gds_types::GdsMessage> mOutMessages;
  binary_semaphore_t                                 mOutSemaphore;
  std::thread                                        mOutMessageThread;

public:
  GDSConnection(const std::string &host, const std::string &port,
                const std::string &path, const GDSCallbacks &GDSMsgCallback,
                const io_service_sptr &io_service);

  GDSConnection(const std::string &url, const GDSCallbacks &GDSMsgCallback,
                const io_service_sptr &io_service);

  GDSConnection(const GDSConnection &) = delete;
  GDSConnection(const GDSConnection &&) = delete;

  GDSConnection &operator=(const GDSConnection &) = delete;
  GDSConnection &operator=(const GDSConnection &&) = delete;
  ~GDSConnection();

  void sendMessage(const gds_lib::gds_types::GdsMessage &msg) override;
  void close() override;

private:
  void onMessage(ws_connection_sptr /*connection*/,
                 std::shared_ptr<ws_client_t::InMessage> message);
  void onOpen(ws_connection_sptr connection);
  void onClose(ws_connection_sptr /*connection*/, int status,
               const std::string &reason);
  void onError(ws_connection_sptr /*connection*/,
               const SimpleWeb::error_code &error_code);

  void inTreadFunc();
  void outTreadFunc();
};

} // namespace connection
} // namespace gds_lib

#endif // GDS_CONNECTION_IMPL_HPP