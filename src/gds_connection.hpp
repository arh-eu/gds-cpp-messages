#ifndef GDS_CONNECTION_HPP
#define GDS_CONNECTION_HPP

#include "gds_types.hpp"

#include <functional>
#include <memory>
#include <string>

namespace gds_lib {
  namespace connection {

    struct GDSInterface {
      virtual ~GDSInterface();

      virtual void close() = 0;
      virtual void start() = 0;
      virtual void send(const gds_lib::gds_types::GdsMessage &msg) = 0;

      std::function<void()> on_open;
      std::function<void(gds_lib::gds_types::GdsMessage &)> on_message;
      std::function<void(int, const std::string&)> on_close;
      std::function<void(int, const std::string&)> on_error;

      static std::shared_ptr<GDSInterface>      create(const std::string &url);
      static std::shared_ptr<GDSInterface>      create(const std::string &url, const std::string& cert, const std::string& cert_pw);
    };

} // namespace connection
} // namespace gds_lib

#endif // GDS_CONNECTION_HPP