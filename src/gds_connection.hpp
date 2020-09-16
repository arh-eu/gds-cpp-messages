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
        virtual void send(const gds_lib::gds_types::GdsMessage& msg) = 0;

        std::function<void()> on_open;
        std::function<void(bool,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>)> on_login;
        std::function<void(gds_lib::gds_types::GdsMessage&)> on_message;
        std::function<void(int, const std::string&)> on_close;
        std::function<void(int, const std::string&)> on_error;

        //NO AUTH
        static std::shared_ptr<GDSInterface> create(const std::string& url, const std::string& username);

        //PASSWORD AUTH
        static std::shared_ptr<GDSInterface> create(const std::string& url, const std::string& username, const std::string& password);

        //TLS AUTH
        static std::shared_ptr<GDSInterface> create(const std::string& url, const std::string& username, const std::string& cert, const std::string& cert_pw);
    };

} // namespace connection
} // namespace gds_lib

#endif // GDS_CONNECTION_HPP