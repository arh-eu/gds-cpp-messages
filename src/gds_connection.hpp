#ifndef GDS_CONNECTION_HPP
#define GDS_CONNECTION_HPP

#include "gds_types.hpp"

#include "asio.hpp"

#include <exception>
#include <functional>
#include <memory>
#include <string>

namespace gds_lib {
namespace connection {

using gds_conn_rdy = std::function<void()>;
using gds_msg_fn = std::function<void(gds_lib::gds_types::GdsMessage &)>;
using gds_msg_err_fn = std::function<void(const std::exception &)>;
using gds_ws_err_fn =
    std::function<void(const gds_lib::gds_types::WebsocketResult &)>;
using gds_ws_closed_fn =
    std::function<void(const gds_lib::gds_types::WebsocketResult &)>;

struct GDSCallbacks {
  gds_conn_rdy   connReadyCallback;
  gds_msg_fn     msgCallback;
  gds_msg_err_fn msgErrorCallback;
  gds_ws_err_fn  wsErrorCallback;
  gds_ws_closed_fn  wsClosedCallback;
};

using io_service_sptr = std::shared_ptr<asio::io_service>;

class GDSInterface {
public:
  virtual ~GDSInterface();
  virtual void close() = 0;
  virtual void sendMessage(const gds_lib::gds_types::GdsMessage &msg) = 0;

  static std::shared_ptr<GDSInterface>
  create(const std::string &host, const std::string &port,
         const std::string &path, const GDSCallbacks &GDSMsgCallback,
         const io_service_sptr &io_service);

  static std::shared_ptr<GDSInterface>
  create(const std::string &url, const GDSCallbacks &GDSMsgCallback,
         const io_service_sptr &io_service);
};

} // namespace connection
} // namespace gds_lib

#endif // GDS_CONNECTION_HPP